#include "lmice_eal_aio.h"

#if defined(__APPLE__)

void* eal_aio_thread(void* pdata) {
    lm_aio_ctx_t* ctx = (lm_aio_ctx_t*)pdata;
    void (*handler) (void*) = ctx->handler;
    volatile int64_t *quit_flag = ctx->quit_flag;
    void* context = ctx->context;

    /* clean data */
    eal_aio_free_context(ctx);

    for(;;) {
        /* check quit flag */
        if(*quit_flag == 1)
            break;

        /* call user function */
        handler(context);
    }
    return 0;
}

#endif
