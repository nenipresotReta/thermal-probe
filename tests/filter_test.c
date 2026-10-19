#include "filter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
 for(int mode=0;mode<3;mode++){struct filter f={0};double v,a,b;for(int i=0;i<FILTER_N+80;i++){double t=i*.1;bool ok=filter_push(&f,t,20+(mode>=1?0.3*t:0)+(mode==2?.02*t*t:0),&v,&a,&b);assert(ok==(i>=FILTER_N-1));if(ok){assert(fabs(a-(mode==0?0:mode==1?.3:.3+.04*t))<1e-8);assert(fabs(b-(mode==2?.04:0))<1e-8);}}assert(!filter_push(&f,100,20,&v,&a,&b));assert(!filter_push(&f,101,NAN,&v,&a,&b));assert(f.n==0);}
 struct filter f={0};double s,d1,d2,rawE=0,fitE=0;double prev=0;for(int i=0;i<1000;i++){double t=i*.1,noise=.1*sin(i*2.3);bool ok=filter_push(&f,t,20+.2*t+noise,&s,&d1,&d2);if(ok){rawE+=pow((noise-prev)/.1,2);fitE+=pow(d1-.2,2);}prev=noise;}assert(fitE<rawE*.1);printf("PASS: constant, linear, quadratic derivatives; warmup; gap; invalid sample; noise attenuation %.4f\n",sqrt(fitE/rawE));
}
