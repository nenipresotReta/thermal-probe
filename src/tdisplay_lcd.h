/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
int tdisplay_lcd_init(void);
void tdisplay_lcd_band(unsigned y,unsigned height,const uint8_t *pixels);
