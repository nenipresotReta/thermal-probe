#ifndef PROBE_FORMAT_H
#define PROBE_FORMAT_H

#include <stddef.h>

/* Serialize application formatting: SDK newlib shares its dtoa work area.
 * These functions are for threads, not ISRs. No sample/display lock is taken. */
int probe_snprintf(char *buffer, size_t size, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

/* Formats at most 255 bytes before releasing the format lock and writing to
 * stdout. Returns -1 on formatting error, truncation, or a short console write. */
int probe_printf(const char *format, ...)
    __attribute__((format(printf, 1, 2)));

#endif
