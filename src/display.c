/* Local display owns the LCD and local curve; sampling/network remain independent. */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <math.h>
#include "format.h"
#include <stdlib.h>
#include <string.h>
#include "tdisplay_lcd.h"
#include "display.h"
#include "drawing.h"
#define HISTORY 600
#define BAND_HEIGHT 4
#define COLOR_BACKGROUND 0x0882
#define COLOR_PLOT 0x10c4
#define COLOR_INK 0xe77e
#define COLOR_MUTED 0x8cb3
#define COLOR_LINE 0x31a7
#define COLOR_ACCENT 0x5d3f
#define COLOR_ERROR 0xf36b
#define COLOR_GREEN 0x3dca
K_MUTEX_DEFINE(display_lock);
static struct {double temperature,d1,d2;bool valid,ready;int64_t at;} latest;
static int16_t values[HISTORY];
static uint32_t times[HISTORY];
static unsigned count,head;
static uint8_t pixels[320*BAND_HEIGHT*2];
static bool running=true;
static int64_t accumulated,started;
void display_sample(double temperature,double d1,double d2,bool valid,bool ready){
 k_mutex_lock(&display_lock,K_FOREVER);
 latest.temperature=temperature;latest.d1=d1;latest.d2=d2;latest.valid=valid;latest.ready=ready;latest.at=k_uptime_get();
 k_mutex_unlock(&display_lock);
}
static void draw(int64_t now){
 double temperature,d1,d2;bool valid,ready;
 k_mutex_lock(&display_lock,K_FOREVER);
 temperature=latest.temperature;d1=latest.d1;d2=latest.d2;valid=latest.valid&&now-latest.at<2000;ready=latest.ready;
 k_mutex_unlock(&display_lock);
 unsigned elapsed=(accumulated+(running?now-started:0))/1000;
 char ip[NET_IPV4_ADDR_LEN]="WIFI CONNECTING";
 struct net_if *iface=net_if_get_default();
 const struct in_addr *addr=iface?net_if_ipv4_get_global_addr(iface,NET_ADDR_PREFERRED):NULL;
 if(addr)net_addr_ntop(AF_INET,addr,ip,sizeof ip);
 double low=0,high=250;
 for(unsigned i=0;i<count;i++){int16_t stored=values[(head+HISTORY-count+i)%HISTORY];double v=stored/10.0;if(stored!=INT16_MIN){if(v<low)low=floor(v/50)*50;if(v>high)high=ceil(v/50)*50;}}
 double first=count?times[(head+HISTORY-count)%HISTORY]:0,last=count?times[(head+HISTORY-1)%HISTORY]:0;
 /* Labels are constant for the whole frame, not recalculated for every band. */
 char timer_text[32],temperature_text[64],d1_text[64],d2_text[64],axis_text[3][32];
 probe_snprintf(timer_text,sizeof timer_text,"%s %02u:%02u",running?"LIVE":"HOLD",elapsed/60,elapsed%60);
 if(valid)probe_snprintf(temperature_text,sizeof temperature_text,"%.1f C",temperature);else probe_snprintf(temperature_text,sizeof temperature_text,"SENSOR ERROR");
 if(valid&&ready)probe_snprintf(d1_text,sizeof d1_text,"D1: %.2f C/MIN",d1*60.0);else probe_snprintf(d1_text,sizeof d1_text,"D1: -- C/MIN");
 if(valid&&ready)probe_snprintf(d2_text,sizeof d2_text,"D2: %.2f C/MIN2",d2*3600.0);else probe_snprintf(d2_text,sizeof d2_text,"D2: -- C/MIN2");
 for(int j=0;j<3;j++)probe_snprintf(axis_text[j],sizeof axis_text[j],"%.0f",high-(high-low)*j/2);
 for(int y=0;y<170;y+=BAND_HEIGHT){
 unsigned height=170-y<BAND_HEIGHT?170-y:BAND_HEIGHT;
 for(unsigned i=0;i<sizeof pixels;i+=2){pixels[i]=COLOR_BACKGROUND>>8;pixels[i+1]=COLOR_BACKGROUND&255;}struct canvas c={pixels,y,height};
 rect(&c,0,49,320,92,COLOR_PLOT);rect(&c,0,13,320,1,COLOR_LINE);rect(&c,161,18,1,24,COLOR_LINE);
 text(&c,5,3,"PT100 / WIFI",1,COLOR_INK);
 text(&c,222,3,timer_text,1,running?COLOR_GREEN:COLOR_MUTED);
 text(&c,5,18,temperature_text,2,valid?COLOR_INK:COLOR_ERROR);
 text(&c,170,20,d1_text,1,COLOR_INK);
 text(&c,170,32,d2_text,1,COLOR_ACCENT);
 for(int j=0;j<3;j++){text(&c,2,53+j*40,axis_text[j],1,COLOR_MUTED);rect(&c,28,56+j*40,288,1,COLOR_LINE);}
 int px=-1,py=0;
 for(unsigned i=0;i<count;i++){
 unsigned index=(head+HISTORY-count+i)%HISTORY;double v=values[index]/10.0;if(values[index]==INT16_MIN){px=-1;continue;}
 int xx=28+(last>first?(int)((times[index]-first)*287/(last-first)):0),yy=136-(v-low)*80/(high-low);
 if(px>=0){int steps=abs(xx-px)>abs(yy-py)?abs(xx-px):abs(yy-py);for(int j=0;j<=steps;j++)rect(&c,steps?px+(xx-px)*j/steps:xx,steps?py+(yy-py)*j/steps:yy,1,1,COLOR_ACCENT);}
 rect(&c,xx,yy,1,1,COLOR_ACCENT);px=xx;py=yy;
 }
 text(&c,5,145,ip,1,addr?COLOR_MUTED:COLOR_ACCENT);
 text(&c,5,159,"10HZ / TAP HOLD / HOLD 2S CLEAR",1,COLOR_MUTED);
 tdisplay_lcd_band(y,height,pixels);
 }
}
static void ui(void *a,void *b,void *c){
 const struct device *gpio=DEVICE_DT_GET(DT_NODELABEL(gpio0));
 int rc=tdisplay_lcd_init();probe_printf("DISPLAY init=%d 320x170\n",rc);if(rc)return;
 rc=gpio_pin_configure(gpio,14,GPIO_INPUT|GPIO_PULL_UP);if(rc){probe_printf("DISPLAY button init=%d\n",rc);return;}
 started=k_uptime_get();int last=1,stable=1;int64_t changed=started,pressed=0,next_draw=started,next_history=started;
 for(;;){int64_t now=k_uptime_get();int button=gpio_pin_get(gpio,14);
 if(button>=0){if(button!=last){last=button;changed=now;}if(button!=stable&&now-changed>=30){stable=button;if(!button)pressed=now;else if(now-pressed>=2000){head=count=0;accumulated=0;started=now;next_history=now;probe_printf("DISPLAY local curve cleared\n");}else{if(running)accumulated+=now-started;else started=now;running=!running;next_history=now;probe_printf("DISPLAY curve=%s; acquisition remains 10Hz\n",running?"LIVE":"HOLD");}}}
 if(running&&now>=next_history){k_mutex_lock(&display_lock,K_FOREVER);values[head]=latest.valid&&now-latest.at<2000?(int16_t)lround(latest.temperature*10):INT16_MIN;k_mutex_unlock(&display_lock);times[head]=(accumulated+now-started)/1000.0;head=(head+1)%HISTORY;if(count<HISTORY)count++;next_history=now+3000;}
 if(now>=next_draw){draw(now);next_draw=k_uptime_get()+500;}
 k_msleep(10);
 }
}
K_THREAD_STACK_DEFINE(ui_stack,4096);
static struct k_thread ui_thread;
void display_start(void){k_thread_create(&ui_thread,ui_stack,K_THREAD_STACK_SIZEOF(ui_stack),ui,NULL,NULL,NULL,12,0,K_NO_WAIT);}
