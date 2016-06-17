#ifndef LMICE_EAL_AIO_H
#define LMICE_EAL_AIO_H

/** Async IO */
#include "lmice_eal_common.h"
#include <stdlib.h>
#include <stdint.h>

struct lmice_aio_context_s {
    /* =1 quit */
    volatile int64_t* quit_flag;
    /* callback function pointer */
    void (*handler) (void*);
    /* callback parameter */
    void* context;
};
typedef struct lmice_aio_context_s lm_aio_ctx_t;

#define eal_aio_malloc_context(ctx) \
    ctx = (lm_aio_ctx_t*)malloc(sizeof(lm_aio_ctx_t));

#define eal_aio_free_context(ctx) \
    free(ctx)

#if defined(_WIN32)
#include "lmice_eal_iocp.h"
#define eal_aio_data_t eal_iocp_data
#define eal_aio_data_list eal_iocp_data_list
#define eal_aio_handle eal_iocp_handle
#define eal_aio_t HANDLE

#define eal_aio_create_handle eal_create_iocp_handle

#define eal_aio_add_read(cp, sock, id) eal_append_iocp_handle(cp, sock, id)

forceinline int eal_aio_create(eal_aio_t* thd, lm_aio_ctx_t* ctx)
{
    (void)thd;
    (void)ctx;
    eal_aio_free_context(ctx);
    return 0;
}

#define eal_aio_destroy(ctx) (void)ctx

#elif defined(__APPLE__)

#include <pthread.h>

#include "lmice_eal_kqueue.h"
#define eal_aio_data_t eal_kqueue_data_t
#define eal_aio_handle eal_kqueue_handle
#define eal_aio_t eal_kqueue_t

#define eal_aio_add_read(evt, sock, id) eal_kqueue_add_read_event(evt, sock, id)

void* eal_aio_thread(void*);

forceinline int eal_aio_create(pthread_t* thd, lm_aio_ctx_t* ctx)
{
    int ret;
    ret = pthread_create(thd, NULL, eal_aio_thread, ctx);
    return ret;
}

#define eal_aio_destroy(ctx) do {\
    ctx->quit_flag = 1; \
    pthread_join(ctx->thd, NULL); \
    } while(0)

forceinline
int eal_aio_create_handle(eal_aio_handle* handle)
{
    *handle = eal_kqueue_create_handle();
    return 0;
}

#elif defined(__LINUX__)
#endif




#endif /* LMICE_EAL_AIO_H */

