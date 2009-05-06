#ifndef FAKEPTHREAD_H_
#define FAKEPTHREAD_H_

#include <signal.h>
#include "bwc_config.h"
#include "macros.h"

// See select_mpi.h for info on my_* stuff.

typedef void * my_pthread_t;
typedef int my_pthread_attr_t;

static inline int my_pthread_create(my_pthread_t * /* restrict */ t, const my_pthread_attr_t * /* restrict */ a MAYBE_UNUSED, void *(*s)(void*), void * arg)
{
    void * u=(*s)(arg);
    *t = u;
    return 0;
}

static inline int my_pthread_join(my_pthread_t t, void ** vp)
{
    if (vp)
        *vp = t;
    return 0;
} 

#define my_pthread_barrier_destroy         pthread_barrier_destroy

typedef int my_pthread_barrier_t;
typedef int my_pthread_barrierattr_t;

static inline int my_pthread_barrier_init(my_pthread_barrier_t * /* restrict */ b MAYBE_UNUSED, const my_pthread_barrierattr_t * /* restrict */ attr MAYBE_UNUSED, unsigned int c)
{
    ASSERT_ALWAYS(c == 1);
    return 0;
}

#define MY_PTHREAD_BARRIER_SERIAL_THREAD   -1
static inline int my_pthread_barrier_wait(my_pthread_barrier_t * b MAYBE_UNUSED)
{
    return MY_PTHREAD_BARRIER_SERIAL_THREAD;
}
static inline int my_pthread_barrier_destroy(my_pthread_barrier_t * b MAYBE_UNUSED)
{
    return 0;
}

typedef int barrier_t;

static inline int barrier_wait(barrier_t * b MAYBE_UNUSED,
        void (*in)(int, void *),
        void (*out)(int, void *), void * arg)
{
    if (in) (*in)(0, arg);
    if (out) (*out)(0, arg);
    return 1;   /* waker returns 1 */
}

static inline int barrier_init (barrier_t *b MAYBE_UNUSED, int c)
{
    ASSERT_ALWAYS(c==1);
    return 0;
}

static inline int barrier_destroy (barrier_t *b MAYBE_UNUSED)
{
    return 0;
}

typedef int my_pthread_mutex_t;
typedef int my_pthread_mutexattr_t;

static inline int my_pthread_mutex_init(my_pthread_mutex_t * /* restrict */ m MAYBE_UNUSED, const my_pthread_mutexattr_t * /* restrict */ attr MAYBE_UNUSED)
{
    return 0;
}

static inline int my_pthread_mutex_lock(my_pthread_mutex_t * m MAYBE_UNUSED)
{
    return 0;
}
static inline int my_pthread_mutex_trylock(my_pthread_mutex_t * m MAYBE_UNUSED)
{
    return 0;
}
static inline int my_pthread_mutex_unlock(my_pthread_mutex_t * m MAYBE_UNUSED)
{
    return 0;
}
static inline int my_pthread_mutex_destroy(my_pthread_mutex_t * m MAYBE_UNUSED)
{
    return 0;
}
static inline int my_pthread_sigmask(int how, const sigset_t * set, sigset_t * oset)
{
    return sigprocmask(how, set, oset);
}

#endif	/* FAKEPTHREAD_H_ */
