#include "modbus_codec.h"
#include <string.h>
static unsigned be16(const uint8_t *p){return (p[0]<<8)|p[1];}
size_t modbus_reply(const uint8_t *q,size_t n,uint8_t *r,const uint16_t regs[8],int valid,int ready){
 if(n<8||n>260||be16(q+2)!=0||be16(q+4)!=n-6)return 0;
 memcpy(r,q,8);unsigned code=0,addr=0,count=0;
 if(q[6]!=1)code=11;
 else if(q[7]!=3&&q[7]!=4)code=1;
 else if(n!=12)code=3;
 else {addr=be16(q+8);count=be16(q+10);
 if(!count||count>125)code=3;
 else if(addr>=8||count>8-addr)code=2;
 else if(!valid||(!ready&&addr+count>4))code=4;}
 if(code){r[4]=0;r[5]=3;r[7]|=128;r[8]=code;return 9;}
 r[4]=0;r[5]=3+count*2;r[8]=count*2;
 for(unsigned i=0;i<count;i++){r[9+i*2]=regs[addr+i]>>8;r[10+i*2]=regs[addr+i];}
 return 9+count*2;
}
