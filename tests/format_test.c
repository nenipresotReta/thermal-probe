#include "format.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

_Thread_local unsigned test_mutex_depth;
static atomic_int active_formatters;
static atomic_int console_writes;

/* Compiled into format.c in place of vsnprintf. This deliberately detects
 * overlap, even on hosts whose own libc already has thread-local dtoa state. */
int test_vsnprintf(char *buffer, size_t size, const char *format, va_list args)
{
    assert(test_mutex_depth == 1);
    assert(atomic_fetch_add(&active_formatters, 1) == 0);
    sched_yield();
    int length = vsnprintf(buffer, size, format, args);
    sched_yield();
    assert(atomic_fetch_sub(&active_formatters, 1) == 1);
    return length;
}

size_t test_fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
    (void)buffer;
    assert(test_mutex_depth == 0);
    assert(stream == stdout && size == 1);
    atomic_fetch_add(&console_writes, 1);
    sched_yield();
    return count;
}

static void *worker(void *argument)
{
    long id = *(long *)argument;
    for (int i = 0; i < 1000; i++) {
        char expected[128], actual[128];
        double temperature = id * 100.0 + i / 1000.0;
        int n = snprintf(expected, sizeof expected,
                         "{\"time_s\":%.3f,\"temperature_c\":%.4f}",
                         i / 10.0, temperature);
        assert(probe_snprintf(actual, sizeof actual,
                              "{\"time_s\":%.3f,\"temperature_c\":%.4f}",
                              i / 10.0, temperature) == n);
        assert(strcmp(actual, expected) == 0);
        assert(probe_printf("PT100 %.3f %.6f\n", temperature, -temperature) > 0);
    }
    return NULL;
}

int main(void)
{
    char tiny[4];
    assert(probe_snprintf(tiny, sizeof tiny, "%s", "abcdef") == 6);
    assert(strcmp(tiny, "abc") == 0);
    assert(probe_snprintf(NULL, 0, "%.3f", 12.5) == 6);
    char boundary[257];
    memset(boundary, 'x', sizeof boundary);
    boundary[255] = '\0';
    assert(probe_printf("%s", boundary) == 255);
    boundary[255] = 'x';
    boundary[256] = '\0';
    assert(probe_printf("%s", boundary) == -1);
    assert(atomic_load(&console_writes) == 1);

    pthread_t threads[6];
    long ids[6];
    for (int i = 0; i < 6; i++) {
        ids[i] = i;
        assert(pthread_create(&threads[i], NULL, worker, &ids[i]) == 0);
    }
    for (int i = 0; i < 6; i++) {
        assert(pthread_join(threads[i], NULL) == 0);
    }
    assert(atomic_load(&console_writes) == 6001);
    assert(atomic_load(&active_formatters) == 0);
    puts("PASS: 12000 serialized concurrent formats; console I/O unlocked; bounds preserved");
}
