#pragma once
#include <stdbool.h>
void wireless_start(void);
void wireless_sample(double raw,double smooth,double d1,double d2,bool valid,bool ready);
