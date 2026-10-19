#include "filter.h"
#include <math.h>
#include <string.h>
/* Causal quadratic least squares, using actual seconds relative to newest sample.
 * Derivatives are evaluated at the newest timestamp (not at window centre). */
bool filter_push(struct filter *f,double t,double v,double *smooth,double *d1,double *d2){
 if(!isfinite(t)||!isfinite(v)){f->n=0;return false;}
 if(f->n && (t<=f->t[f->n-1] || t-f->t[f->n-1]>1.5))f->n=0;
 if(f->n==FILTER_N){memmove(f->t,f->t+1,(FILTER_N-1)*sizeof(double));memmove(f->v,f->v+1,(FILTER_N-1)*sizeof(double));f->n--;}
 f->t[f->n]=t;f->v[f->n++]=v;*smooth=v;
 if(f->n<FILTER_N)return false;
 double a[3][4]={0};
 for(int k=0;k<f->n;k++){double x=f->t[k]-t,p[5]={1};for(int j=1;j<5;j++)p[j]=p[j-1]*x;for(int i=0;i<3;i++){for(int j=0;j<3;j++)a[i][j]+=p[i+j];a[i][3]+=p[i]*f->v[k];}}
 for(int i=0;i<3;i++){int best=i;for(int k=i+1;k<3;k++)if(fabs(a[k][i])>fabs(a[best][i]))best=k;for(int j=0;j<4;j++){double tmp=a[i][j];a[i][j]=a[best][j];a[best][j]=tmp;}double div=a[i][i];if(fabs(div)<1e-10)return false;for(int j=i;j<4;j++)a[i][j]/=div;for(int k=0;k<3;k++)if(k!=i){double q=a[k][i];for(int j=i;j<4;j++)a[k][j]-=q*a[i][j];}}
 *smooth=a[0][3];*d1=a[1][3];*d2=2*a[2][3];return true;
}
