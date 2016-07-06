#ifndef LMICE_EAL_THREAD_PTHREAD_H
#define LMICE_EAL_THREAD_PTHREAD_H

#include <pthread.h>
#include <stdint.h>

#include "lmice_eal_common.h"

#define eal_tls_t pthread_key_t

#define eal_thread_t pthread_t

void* eal_thread_thread(void*);

forceinline int eal_thread_create(eal_thread_t* thread, lm_thread_ctx_t* ctx)
{
    int ret = 0;
    ret = pthread_create(thread, NULL, eal_thread_thread, ctx);
    return ret;
}
#define eal_thread_join(trd, ret) do {\
    ret = pthread_join(trd, NULL); \
    } while(0)

#define eal_create_tls(key) pthread_key_create(&key, NULL)
#define eal_get_tls_value(key, type, val) val=(type)pthread_getspecific(key)
#define eal_set_tls_value(key, val) pthread_setspecific(key, val)
#define eal_delete_tls(key) pthread_key_delete(key)
#endif /** LMICE_EAL_THREAD_PTHREAD_H */
