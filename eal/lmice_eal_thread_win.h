#ifndef LMICE_EAL_THREAD_WIN_H
#define LMICE_EAL_THREAD_WIN_H

#include "lmice_eal_common.h"

#define eal_thread __declspec(Thread)
#define eal_tls_t DWORD
#define eal_thread_t HANDLE
forceinline eal_pid_t eal_gettid(void)
{
    return GetCurrentThreadId();
}

forceinline eal_pid_t eal_thisthread_id(void)
{
    return GetCurrentThreadId();
}

int forceinline pthread_getname_np(DWORD tid, char* name, size_t sz)
{
    UNREFERENCED_PARAM(tid);
    UNREFERENCED_PARAM(name);
    UNREFERENCED_PARAM(sz);
    /* No implementation*/
    return -1;
}

int forceinline pthread_setname_np(const char* name)
{
    UNREFERENCED_PARAM(name);
    return 0;
}

eal_pid_t forceinline getpid(void)
{
    return GetCurrentProcessId();
}

DWORD WINAPI eal_thread_thread( LPVOID lpThreadParameter );

forceinline int eal_thread_create(HANDLE* thread, lm_thread_ctx_t* ctx)
{
    *thread = CreateThread(NULL, 0, eal_thread_thread, ctx, 0, NULL);
    if(*thread)
        return 0;
    return 1;
}
#define eal_create_tls(key) *key=TlsAlloc()
#define eal_get_tls_value(key, type, val) val=(type)TlsGetValue(key)
#define eal_set_tls_value(key, val) TlsSetValue(key, val)
#define eal_delete_tls(key) TlsFree(key)

#define eal_thread_join(trd, ret) do {\
    (void)trd;  \
    (void)ret;  \
    }while(0)

#endif /** LMICE_EAL_THREAD_WIN_H */

