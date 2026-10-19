#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_stats.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <esp_wifi.h>
#include "wireless.h"
#include "modbus_codec.h"
#include "wifi_credentials.h"
K_MUTEX_DEFINE(sample_lock);
static struct {float value[4];bool valid,ready;int64_t at;} sample;
static struct net_mgmt_event_callback events, ip_events;
void wireless_sample(double raw,double smooth,double d1,double d2,bool valid,bool ready){
 k_mutex_lock(&sample_lock,K_FOREVER);
 sample.value[0]=raw;sample.value[1]=smooth;sample.value[2]=d1;sample.value[3]=d2;
 sample.valid=valid;sample.ready=ready;sample.at=k_uptime_get();k_mutex_unlock(&sample_lock);
}
static void event(struct net_mgmt_event_callback *cb,uint32_t ev,struct net_if *iface){
 if(ev==NET_EVENT_IPV4_ADDR_ADD){char ip[NET_IPV4_ADDR_LEN];const struct in_addr *a=net_if_ipv4_get_global_addr(iface,NET_ADDR_PREFERRED);
 if(a)printf("WIFI IP=%s MODBUS=502 HTTP=80\n",net_addr_ntop(AF_INET,a,ip,sizeof ip));}
 else if(ev==NET_EVENT_WIFI_CONNECT_RESULT){const struct wifi_status *s=cb->info;printf("WIFI connect status=%d\n",s?s->status:-1);}
 else if(ev==NET_EVENT_WIFI_DISCONNECT_RESULT)printf("WIFI disconnected; reconnecting\n");
}
static int receive_all(int fd,uint8_t *p,size_t n){while(n){int k=zsock_recv(fd,p,n,0);if(k<=0)return -1;p+=k;n-=k;}return 0;}
static int send_all(int fd,const void *buf,size_t n){const char *p=buf;while(n){int k=zsock_send(fd,p,n,0);if(k<=0)return -1;p+=k;n-=k;}return 0;}
static void client(int fd,bool http){
 struct timeval tv={.tv_sec=3};
 if(zsock_setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv)||zsock_setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv)){printf("client timeout setup failed errno=%d\n",errno);return;}
 if(http){char request[512],body[512],response[800];int n=zsock_recv(fd,request,sizeof(request)-1,0);if(n<=0)return;request[n]=0;
 if(strncmp(request,"GET /api/temperature ",21)&&strncmp(request,"GET / ",6)){const char *e="HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";send_all(fd,e,strlen(e));return;}
 k_mutex_lock(&sample_lock,K_FOREVER);bool v=sample.valid&&k_uptime_get()-sample.at<2000;bool r=v&&sample.ready;
 char raw[32]="null",sm[32]="null",d1[32]="null",d2[32]="null";
 if(v){snprintf(raw,sizeof raw,"%.4f",(double)sample.value[0]);snprintf(sm,sizeof sm,"%.4f",(double)sample.value[1]);}
 if(r){snprintf(d1,sizeof d1,"%.6f",(double)sample.value[2]);snprintf(d2,sizeof d2,"%.7f",(double)sample.value[3]);}
 snprintf(body,sizeof body,"{\"valid\":%s,\"derivatives_ready\":%s,\"time_s\":%.3f,\"raw_c\":%s,\"temperature_c\":%s,\"d1_c_per_s\":%s,\"d2_c_per_s2\":%s}\n",v?"true":"false",r?"true":"false",sample.at/1000.0,raw,sm,d1,d2);k_mutex_unlock(&sample_lock);
 n=snprintf(response,sizeof response,"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: %u\r\n\r\n%s",(unsigned)strlen(body),body);send_all(fd,response,n);return;}
 for(;;){uint8_t q[260],reply[260];if(receive_all(fd,q,7))return;unsigned length=(q[4]<<8)|q[5];if(length<2||length>254||q[2]||q[3])return;if(receive_all(fd,q+7,length-1))return;
 uint16_t regs[8];k_mutex_lock(&sample_lock,K_FOREVER);for(int i=0;i<4;i++){uint32_t bits;memcpy(&bits,&sample.value[i],4);regs[2*i]=bits>>16;regs[2*i+1]=bits;}
 bool v=sample.valid&&k_uptime_get()-sample.at<2000;size_t n=modbus_reply(q,length+6,reply,regs,v,sample.ready);k_mutex_unlock(&sample_lock);if(!n||send_all(fd,reply,n))return;}
}
/* Bounded pools: listener never waits for a client to finish a request.
 * Each worker owns one accepted fd until close; queue/semaphore lifetimes
 * prevent both fd reuse races and unbounded connection memory growth. */
#define MODBUS_CLIENTS 4
#define HTTP_CLIENTS 2
K_MSGQ_DEFINE(modbus_connections,sizeof(int),MODBUS_CLIENTS,4);
K_MSGQ_DEFINE(http_connections,sizeof(int),HTTP_CLIENTS,4);
K_SEM_DEFINE(modbus_slots,MODBUS_CLIENTS,MODBUS_CLIENTS);
K_SEM_DEFINE(http_slots,HTTP_CLIENTS,HTTP_CLIENTS);
static void worker(void *port_arg,void *unused,void *unused2){
 bool http=(int)(intptr_t)port_arg==80;
 struct k_msgq *queue=http?&http_connections:&modbus_connections;
 struct k_sem *slots=http?&http_slots:&modbus_slots;
 for(;;){int fd;k_msgq_get(queue,&fd,K_FOREVER);client(fd,http);zsock_close(fd);k_sem_give(slots);}
}
static void server(void *port_arg,void *unused,void *unused2){
 int port=(int)(intptr_t)port_arg;bool http=port==80;
 struct k_msgq *queue=http?&http_connections:&modbus_connections;
 struct k_sem *slots=http?&http_slots:&modbus_slots;
 for(;;){int s=zsock_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
 if(s<0){k_sleep(K_SECONDS(2));continue;}
 int yes=1;zsock_setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes);
 struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(port),.sin_addr.s_addr=INADDR_ANY};
 if(zsock_bind(s,(struct sockaddr *)&a,sizeof a)||zsock_listen(s,http?HTTP_CLIENTS:MODBUS_CLIENTS)){
 printf("listen %d error=%d\n",port,errno);zsock_close(s);k_sleep(K_SECONDS(2));continue;}
 printf("Listening TCP %d clients=%d\n",port,http?HTTP_CLIENTS:MODBUS_CLIENTS);
 for(;;){int fd=zsock_accept(s,NULL,NULL);
 if(fd<0){k_sleep(K_MSEC(100));continue;}
 if(k_sem_take(slots,K_NO_WAIT)){zsock_close(fd);continue;}
 /* Small request/response traffic must not wait for Nagle aggregation. */
 zsock_setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof yes);
 if(k_msgq_put(queue,&fd,K_NO_WAIT)){zsock_close(fd);k_sem_give(slots);}
 }}
}
K_THREAD_DEFINE(modbus_listener,2048,server,(void *)502,NULL,NULL,8,0,2000);
K_THREAD_DEFINE(http_listener,2048,server,(void *)80,NULL,NULL,8,0,2000);
K_THREAD_DEFINE(modbus_worker0,4096,worker,(void *)502,NULL,NULL,8,0,0);
K_THREAD_DEFINE(modbus_worker1,4096,worker,(void *)502,NULL,NULL,8,0,0);
K_THREAD_DEFINE(modbus_worker2,4096,worker,(void *)502,NULL,NULL,8,0,0);
K_THREAD_DEFINE(modbus_worker3,4096,worker,(void *)502,NULL,NULL,8,0,0);
K_THREAD_DEFINE(http_worker0,4096,worker,(void *)80,NULL,NULL,8,0,0);
K_THREAD_DEFINE(http_worker1,4096,worker,(void *)80,NULL,NULL,8,0,0);
void wireless_start(void){
 struct net_if *iface=net_if_get_default();
 net_mgmt_init_event_callback(&events,event,NET_EVENT_WIFI_CONNECT_RESULT|NET_EVENT_WIFI_DISCONNECT_RESULT);net_mgmt_add_event_callback(&events);
 net_mgmt_init_event_callback(&ip_events,event,NET_EVENT_IPV4_ADDR_ADD);net_mgmt_add_event_callback(&ip_events);
 struct wifi_connect_req_params p={0};p.ssid=(const uint8_t *)WIFI_SSID;p.ssid_length=strlen(WIFI_SSID);p.psk=(const uint8_t *)WIFI_PASSWORD;p.psk_length=strlen(WIFI_PASSWORD);p.security=WIFI_SECURITY_TYPE_PSK;p.channel=WIFI_CHANNEL_ANY;p.band=WIFI_FREQ_BAND_2_4_GHZ;p.timeout=30;
 int r=net_mgmt(NET_REQUEST_WIFI_CONNECT,iface,&p,sizeof p);printf("WIFI starting status=%d\n",r);
 /* Continuous telemetry favours predictable latency over modem sleep. */
 int ps=esp_wifi_set_ps(WIFI_PS_NONE);printf("WIFI power_save=off status=%d\n",ps);
}

static void wifi_monitor(void *a,void *b,void *c){for(;;){struct net_if *iface=net_if_get_default();struct wifi_iface_status status={0};int rc=net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS,iface,&status,sizeof status);char ip[NET_IPV4_ADDR_LEN];const struct in_addr *addr=net_if_ipv4_get_global_addr(iface,NET_ADDR_PREFERRED);printf("WIFI status rc=%d state=%d rssi=%d IP=%s\n",rc,status.state,status.rssi,addr?net_addr_ntop(AF_INET,addr,ip,sizeof ip):"none");struct net_stats ns={0};int sr=net_mgmt(NET_REQUEST_STATS_GET_ALL,iface,&ns,sizeof ns);printf("NET stats rc=%d rx_bytes=%u tx_bytes=%u errors=%u tcp_rx=%u tcp_tx=%u\n",sr,ns.bytes.received,ns.bytes.sent,ns.processing_error,ns.tcp.recv,ns.tcp.sent);printf("TCP detail drop=%u segdrop=%u chkerr=%u conndrop=%u rst=%u retrans=%u ipdrop=%u ipchk=%u\n",ns.tcp.drop,ns.tcp.seg_drop,ns.tcp.chkerr,ns.tcp.conndrop,ns.tcp.connrst,ns.tcp.rexmit,ns.ipv4.drop,ns.ip_errors.chkerr);if(addr){struct in_addr gw=net_if_ipv4_get_gw(iface),mask=net_if_ipv4_get_netmask_by_addr(iface,addr);char gb[NET_IPV4_ADDR_LEN],mb[NET_IPV4_ADDR_LEN];printf("NET route gateway=%s mask=%s\n",net_addr_ntop(AF_INET,&gw,gb,sizeof gb),net_addr_ntop(AF_INET,&mask,mb,sizeof mb));}k_sleep(K_SECONDS(10));}}
K_THREAD_DEFINE(wifi_monitor_thread,2048,wifi_monitor,NULL,NULL,NULL,9,0,10000);
