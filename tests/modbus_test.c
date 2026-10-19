#include "modbus_codec.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint8_t q[]={0x12,0x34,0,0,0,6,1,3,0,0,0,2},r[260];
 uint16_t regs[]={0x41c8,0,0x41c8,0,0,0,0,0};
 assert(modbus_reply(q,12,r,regs,1,1)==13);assert(r[0]==0x12&&r[1]==0x34&&r[9]==0x41&&r[10]==0xc8);
 assert(modbus_reply(q,12,r,regs,0,1)==9&&r[8]==4);
 q[9]=4;assert(modbus_reply(q,12,r,regs,1,0)==9&&r[8]==4);
 q[9]=7;assert(modbus_reply(q,12,r,regs,1,1)==9&&r[8]==2);
 q[9]=0;q[7]=6;assert(modbus_reply(q,12,r,regs,1,1)==9&&r[8]==1);
 q[7]=4;assert(modbus_reply(q,12,r,regs,1,1)==13);
 q[5]=5;assert(modbus_reply(q,12,r,regs,1,1)==0);
 puts("Modbus framing, float order, error and warmup tests passed");
}
