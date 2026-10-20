#ifndef TEST_ZEPHYR_KERNEL_H
#define TEST_ZEPHYR_KERNEL_H

#include <assert.h>
#include <pthread.h>

#define K_FOREVER (-1)
#define K_MUTEX_DEFINE(name) pthread_mutex_t name = PTHREAD_MUTEX_INITIALIZER
extern _Thread_local unsigned test_mutex_depth;

static inline void k_mutex_lock(pthread_mutex_t *mutex, int timeout)
{
    assert(timeout == K_FOREVER);
    assert(pthread_mutex_lock(mutex) == 0);
    test_mutex_depth++;
}

static inline void k_mutex_unlock(pthread_mutex_t *mutex)
{
    assert(test_mutex_depth == 1);
    test_mutex_depth--;
    assert(pthread_mutex_unlock(mutex) == 0);
}

#endif
