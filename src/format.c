#include "format.h"
#include <zephyr/kernel.h>
#include <stdarg.h>
#include <stdio.h>

K_MUTEX_DEFINE(format_lock);

static int format_buffer(char *buffer, size_t size, const char *format, va_list args)
{
    /* printf's stdout lock does not protect snprintf's separate FILE object.
     * Both nevertheless use the same newlib _impure_ptr/dtoa scratch state. */
    k_mutex_lock(&format_lock, K_FOREVER);
    int length = vsnprintf(buffer, size, format, args);
    k_mutex_unlock(&format_lock);
    return length;
}

int probe_snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int length = format_buffer(buffer, size, format, args);
    va_end(args);
    return length;
}

int probe_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    int length = format_buffer(buffer, sizeof buffer, format, args);
    va_end(args);
    if (length < 0 || (size_t)length >= sizeof buffer) {
        return -1;
    }
    /* USB console can wait for the host. Never hold the formatting lock
     * during console I/O, so a disconnected console cannot block HTTP/UI. */
    return fwrite(buffer, 1, (size_t)length, stdout) == (size_t)length ? length : -1;
}
