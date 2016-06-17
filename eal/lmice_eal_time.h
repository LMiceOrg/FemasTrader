#ifndef LMICE_EAL_TIME_H
#define LMICE_EAL_TIME_H
#include "lmice_eal_common.h"

#include <time.h>
#include <stdint.h>



struct lmice_timer_context_s {
    /* 100x nano-seconds */
    uint64_t interval;
    /* =1 quit */
    volatile int64_t* quit_flag;
    /* callback function pointer */
    void (*handler) (void*);
    /* callback parameter */
    void* context;
};
typedef struct lmice_timer_context_s lm_timer_ctx_t;

#define eal_timer_malloc_context(ctx) \
    ctx = (lm_timer_ctx_t*)malloc(sizeof(lm_timer_ctx_t));

#define eal_timer_free_context(ctx) \
    free(ctx); \


#if defined(__MACH__)

#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h> /* mach_absolute_time */
#include <pthread.h>
/* GCD */
#include <dispatch/dispatch.h>

forceinline int eal_time_factor(uint64_t* factor) {
    mach_timebase_info_data_t time_base;
    mach_timebase_info(&time_base);
    *factor = time_base.numer / time_base.denom;
    return 0;
}

/* the UTC time since 1970-01-01 */
forceinline int  get_system_time(int64_t* t) {
    uint64_t tick = 0;
    tick = mach_absolute_time();
    *t = tick / 100llu;
    return 0;
}

void* eal_timer_thread(void*);


#define eal_timer_destroy2(ctx) do {\
    ctx->quit_flag = 1; \
    pthread_join(ctx->timer, NULL); \
    } while(0)

forceinline int eal_timer_create2(pthread_t* timer, lm_timer_ctx_t* ctx)
{
    return pthread_create(timer, NULL, eal_timer_thread, ctx);
}

forceinline int eal_timer_create_dispatch(dispatch_source_t* timer, const char* name, uint64_t *interval, uint64_t *leeway, dispatch_function_t handler, void* context)
{
    dispatch_queue_t queue;
    queue = dispatch_queue_create(name, NULL);
    *timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                                    0, 0, queue);
    dispatch_source_set_event_handler_f(*timer, handler);
    dispatch_set_context(*timer, context);
    dispatch_source_set_timer(*timer, dispatch_walltime(NULL, 0),
                              *interval * 100llu, *leeway * 100llu);
    dispatch_resume(*timer);

    return 0;
}

forceinline int eal_timer_delete(dispatch_source_t timer)
{
    dispatch_release(timer);
    return 0;
}

#elif defined(__linux__) || defined(__MINGW32__)

#include <unistd.h>
#include <time.h>
#include <stdint.h>

/*POSIX.1-2001*/
forceinline int  get_system_time(int64_t* t)
{
    int ret = 0;
    struct timespec tp;
    ret = clock_gettime(CLOCK_REALTIME, &tp);
    if(ret == 0)
    {
        *t = (int64_t)tp.tv_sec*10000000LL + tp.tv_nsec/100LL;
    }
    return ret;

}

#elif defined(_WIN32) && !defined(__MINGW32__)
#include "lmice_eal_time_win.h"
#else
#error("No time implementation!")
#endif

#endif /** LMICE_EAL_TIME_H */

