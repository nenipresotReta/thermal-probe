#pragma once
#include <stdbool.h>
#define FILTER_N 101 /* 10 seconds at 10 Hz, including both endpoints. */
struct filter { double t[FILTER_N],v[FILTER_N]; int n; };
bool filter_push(struct filter *f,double t,double v,double *smooth,double *d1,double *d2);
