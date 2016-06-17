#include "lmice_eal_thread.h"

#if defined(__APPLE__)

void* eal_thread_thread(void* pdata)
{
    lm_thread_ctx_t* ctx = (lm_thread_ctx_t*) pdata;
    eal_thread_callback handler = ctx->handler;
    void* context = ctx->context;


    /* free pdata */
    eal_thread_free_context(pdata);

    /* call user-function */
    handler(context);

    return 0;
}
#elif defined(_WIN32)

DWORD WINAPI eal_thread_thread( LPVOID pdata ) {
    lm_thread_ctx_t* ctx = (lm_thread_ctx_t*) pdata;
    eal_thread_callback handler = ctx->handler;
    void* context = ctx->context;


    /* free pdata */
    eal_thread_free_context(pdata);

    /* call user-function */
    handler(context);

    return 0;
}

#endif
