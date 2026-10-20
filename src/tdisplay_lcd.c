/* SPDX-License-Identifier: MIT
 * ST7789 8-bit 8080 bus, T-Display S3 pinout. Single UI-thread owner.
 * GPIO bank writes avoid a slow per-pin API on each pixel. No IRQ masking:
 * the display permits idle pauses between strobes, so BLE/USB keep running.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc/gpio_struct.h>
#include "tdisplay_lcd.h"
#define ADV_WIDTH 320
#define ADV_HEIGHT 170
static const struct device *g0=DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *g1=DEVICE_DT_GET(DT_NODELABEL(gpio1));
static const unsigned data_pins[]={39,40,41,42,45,46,47,48};
static uint32_t data_bits[256];
#define DATA_MASK ((15u<<7)|(15u<<13))
static inline void level(unsigned pin,bool high) {
    if(pin<32){if(high)GPIO.out_w1ts=1u<<pin;else GPIO.out_w1tc=1u<<pin;}
    else {if(high)GPIO.out1_w1ts.val=1u<<(pin-32);else GPIO.out1_w1tc.val=1u<<(pin-32);}
}
static inline void byte(uint8_t b) {
    level(8,false);GPIO.out1_w1tc.val=DATA_MASK;GPIO.out1_w1ts.val=data_bits[b];
    __asm__ volatile("memw; nop; nop; nop" ::: "memory");
    level(8,true);
    __asm__ volatile("memw; nop; nop; nop" ::: "memory");
}
static void command(uint8_t c,const uint8_t *data,unsigned n) {
    level(6,false);level(7,false);byte(c);level(7,true);
    for(unsigned i=0;i<n;i++)byte(data[i]);
    level(6,true);
}
static void window(unsigned x,unsigned y,unsigned w,unsigned h) {
    /* Landscape: visible rows begin at controller row 35. */
    y+=35;
    uint8_t cols[]={x>>8,x,(x+w-1)>>8,x+w-1};
    uint8_t rows[]={y>>8,y,(y+h-1)>>8,y+h-1};
    command(0x2a,cols,4);command(0x2b,rows,4);command(0x2c,NULL,0);
}
int tdisplay_lcd_init(void) {
    if(!device_is_ready(g0)||!device_is_ready(g1))return -ENODEV;
    const unsigned controls[]={5,6,7,8,9,15,38};
    for(unsigned i=0;i<sizeof(controls)/sizeof(*controls);i++) {
        unsigned p=controls[i];int e=gpio_pin_configure(p<32?g0:g1,p%32,GPIO_OUTPUT_HIGH);if(e)return e;
    }
    level(38,false);
    for(unsigned i=0;i<8;i++){int e=gpio_pin_configure(g1,data_pins[i]-32,GPIO_OUTPUT_LOW);if(e)return e;}
    for(unsigned b=0;b<256;b++)for(unsigned i=0;i<8;i++)if(b&(1u<<i))data_bits[b]|=1u<<(data_pins[i]-32);
    level(5,false);k_msleep(20);level(5,true);k_msleep(120);
    command(0x01,NULL,0);k_msleep(150);command(0x11,NULL,0);k_msleep(120);
    const uint8_t fmt=0x55,rotation=0x60;
    command(0x3a,&fmt,1);command(0x36,&rotation,1);
    command(0x21,NULL,0);command(0x13,NULL,0);command(0x29,NULL,0);k_msleep(20);
    window(0,0,320,170);level(6,false);
    for(unsigned i=0;i<320*170;i++){byte(0);byte(0);}
    level(6,true);level(38,true);return 0;
}
void tdisplay_lcd_band(unsigned y,unsigned height,const uint8_t *pixels) {
    window((320-ADV_WIDTH)/2,(170-ADV_HEIGHT)/2+y,ADV_WIDTH,height);
    level(6,false);
    for(unsigned i=0;i<ADV_WIDTH*height*2;i++)byte(pixels[i]);
    level(6,true);
}
