#pragma once
#include <stddef.h>
#include <stdint.h>
/* Complete MBAP-framed request to response; snapshot = four big-endian floats. */
size_t modbus_reply(const uint8_t *q, size_t n, uint8_t *r, const uint16_t regs[8], int valid, int ready);
