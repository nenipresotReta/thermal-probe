#pragma once
#include <stdbool.h>
#ifdef CONFIG_THERMAL_DISPLAY
void display_start(void);
void display_sample(double temperature,double d1,double d2,bool valid,bool ready);
#else
static inline void display_start(void) {}
static inline void display_sample(double temperature,double d1,double d2,bool valid,bool ready) {
 (void)temperature;(void)d1;(void)d2;(void)valid;(void)ready;
}
#endif
