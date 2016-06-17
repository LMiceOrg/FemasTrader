#ifndef LMICE_EAL_THREAD_H
#define LMICE_EAL_THREAD_H

#include "lmice_eal_common.h"

#include <stdint.h>
#include <stdlib.h>

typedef void(*eal_thread_callback)(void*);
struct lmice_thread_context_s
{
    /* callback function pointer */
    eal_thread_callback handler;
    /* callback parameter */
    void* context;
};
typedef struct lmice_thread_context_s lm_thread_ctx_t;

#define eal_thread_malloc_context(ctx) \
    ctx = (lm_thread_ctx_t*)malloc(sizeof(lm_thread_ctx_t));

#define eal_thread_free_context(ctx) \
    free(ctx); \


#if defined(_WIN32)
typedef uint32_t eal_tid_t;
typedef int32_t  eal_pid_t;

#elif defined(__APPLE__) || defined(__linux__)
#define eal_tid_t pthread_t
#define eal_pid_t int32_t
#else
typedef uint64_t eal_tid_t;
typedef int32_t  eal_pid_t;

#endif

#if defined(__linux__) || defined(USE_POSIX_THREAD) || defined(__APPLE__)

#include "lmice_eal_thread_pthread.h"

forceinline eal_tid_t eal_gettid()
{
    return pthread_self();
}


#elif defined(_WIN32) && defined(_MSC_VER) /** MSC */
    #include "lmice_eal_thread_win.h"


#elif defined(__MINGW32__)
#include "lmice_eal_thread_pthread.h"

static forceinline  int pthread_getname_np(DWORD tid, char* name, size_t sz)
{
    UNREFERENCED_PARAM(tid);
    UNREFERENCED_PARAM(name);
    UNREFERENCED_PARAM(sz);
    /* No implementation*/
    return -1;
}

forceinline static int pthread_setname_np(const char* name)
{
    UNREFERENCED_PARAM(name);
    return 0;
}

#else
    #error(Unsupported thread library)
#endif

#endif /** LMICE_EAL_THREAD_H */

