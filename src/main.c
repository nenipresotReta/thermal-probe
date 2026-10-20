#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wireless.h"
#include "filter.h"
#include "display.h"

static const struct device *gpio=DEVICE_DT_GET(DT_NODELABEL(gpio0));
enum {CS=10,MOSI=11,SCK=12,MISO=13,BUTTON=14};
static void set(int p,int v){gpio_pin_set(gpio,p,v);}
/* MAX31865 SPI mode 1: clock idle low, data sampled on falling edge. */
static unsigned transfer(unsigned v){unsigned r=0;for(int i=7;i>=0;i--){set(SCK,1);set(MOSI,(v>>i)&1);k_busy_wait(2);set(SCK,0);k_busy_wait(2);r=(r<<1)|(gpio_pin_get(gpio,MISO)>0);}return r;}
static void write_reg(unsigned addr,unsigned val){set(CS,0);transfer(addr|128);transfer(val);set(CS,1);}
static unsigned read_reg(unsigned addr){set(CS,0);transfer(addr);unsigned r=transfer(0);set(CS,1);return r;}
static bool temperature(double *t){
 /* Bias + continuous + 3-wire + 50 Hz mains rejection. Readback catches missing chip. */
 if((read_reg(0)&0xd1)!=0xd1){write_reg(0,0xd3);return false;}
 set(CS,0);transfer(1);unsigned raw=transfer(0)<<8;raw|=transfer(0);set(CS,1);
 if((raw&1)||read_reg(7)){write_reg(0,0xd3);return false;}
 double r=(raw>>1)*430.0/32768.0;
 if(r<18.5||r>390.5)return false;
 /* Invert IEC 60751 including the negative-temperature C term. */
 double lo=-200,hi=850;
 for(int i=0;i<32;i++){double x=(lo+hi)/2;double rr=100*(1+3.9083e-3*x-5.775e-7*x*x+(x<0?-4.183e-12*(x-100)*x*x*x:0));if(rr<r)lo=x;else hi=x;}
 *t=(lo+hi)/2;return true;
}
int main(void){
 gpio_pin_configure(gpio,15,GPIO_OUTPUT_HIGH);
 gpio_pin_configure(gpio,CS,GPIO_OUTPUT_HIGH);gpio_pin_configure(gpio,MOSI,GPIO_OUTPUT_LOW);gpio_pin_configure(gpio,SCK,GPIO_OUTPUT_LOW);gpio_pin_configure(gpio,MISO,GPIO_INPUT|GPIO_PULL_DOWN);
 write_reg(3,0xff);write_reg(4,0xff);write_reg(5,0);write_reg(6,0);write_reg(0,0xd3);k_msleep(100);
 wireless_start();display_start();struct filter f={0};int64_t next=k_uptime_get();
 for(;;){int64_t now=k_uptime_get();double raw=NAN,smooth=NAN,d1=NAN,d2=NAN;bool valid=temperature(&raw),ready=false;
 if(valid){smooth=raw;ready=filter_push(&f,now/1000.0,raw,&smooth,&d1,&d2);}else f.n=0;
 wireless_sample(raw,smooth,d1,d2,valid,ready);
 display_sample(smooth,d1,d2,valid,ready);
 if(valid)printf("PT100 t_ms=%lld raw=%.3f C smooth=%.3f C d1=%.5f d2=%.6f ready=%d\n",(long long)now,raw,smooth,d1,d2,ready);else printf("PT100 SENSOR_ERROR\n");
 next+=100;if(next<=k_uptime_get())next=k_uptime_get()+100;k_sleep(K_TIMEOUT_ABS_MS(next));}
}
