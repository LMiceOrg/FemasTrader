#include "lmice_eal_time.h"
#include "lmice_trace.h"

#include <stdlib.h>

#if defined(__MACH__)

void* eal_timer_thread(void * data)
{

    lm_timer_ctx_t *ctx = (lm_timer_ctx_t*)data;

    uint64_t factor = 1;
    uint64_t time_to_wait = 0;
    uint64_t now = 0;
    uint64_t interval = ctx->interval;

    void (*handler) (void*) = ctx->handler;
    volatile int64_t *quit_flag = ctx->quit_flag;
    void* context = ctx->context;

    /* clean data */
    eal_timer_free_context(ctx);

    /* init factor  of nano second */
    eal_time_factor(&factor);

    /* from 100x nano-seconds to nano-seconds */
    time_to_wait = interval * 100llu * factor;
    interval = time_to_wait;

    /* for-loop to work */
    for(;;) {
        if(*quit_flag == 1)
            break;

        now = mach_absolute_time() + time_to_wait;
        mach_wait_until(now);

        /* call user function */
        handler(context);

        time_to_wait = now + interval - mach_absolute_time();
        if(time_to_wait > interval)
            time_to_wait = 0;
    }

    return 0;
}

#elif defined(_WIN32)

#if defined(USE_QUERY_PERFORMANCE_COUNTER)

DWORD WINAPI eal_timer_thread(LPVOID data)
{
    DWORD err = 0;
    BOOL bret = 0;
    LARGE_INTEGER factor;
    LARGE_INTEGER time_to_wait;
    LARGE_INTEGER now;
    LARGE_INTEGER interval;
    LARGE_INTEGER count_per_ms;

    /* assign data context */
    lm_timer_ctx_t *ctx = (lm_timer_ctx_t*)data;
    void (*handler) (void*) = ctx->handler;
    volatile int64_t *quit_flag = ctx->quit_flag;
    void* context = ctx->context;
    interval.QuadPart = ctx->interval;

    /* clean data */
    eal_timer_free_context(ctx);

    /* init factor count-per-second */
    bret = QueryPerformanceFrequency(&factor);
    if(bret == 0)
    {
        err = GetLastError();
        lmice_error_print("eal_timer_thread call QueryPerformanceFrequency failed[%lu]\n", err);
        return 1;
    }

    /*
     * lmice_error_print("eal_timer_thread interval [%lld] count, factor [%lld]\n", interval.QuadPart, factor.QuadPart
                      );
    */
    count_per_ms.QuadPart = (LONGLONG) ( (double)factor.QuadPart / 1000LL );

    /* from 100-nano-seconds to count-per-second */
    interval.QuadPart = (LONGLONG) ( (double)interval.QuadPart
                                     * (double)factor.QuadPart / 10000000LL );

    QueryPerformanceCounter(&time_to_wait);
    time_to_wait.QuadPart += interval.QuadPart;

    /*
    lmice_error_print("eal_timer_thread interval [%lld] count, count_per_ms [%lld]\n", interval.QuadPart, count_per_ms.QuadPart
                      );
    */
    /* for-loop to work */
    for(;;) {
        if(*quit_flag == 1)
            break;

        bret = QueryPerformanceCounter(&now);
        if(bret == 0)
        {
            err = GetLastError();
            lmice_error_print("eal_timer_thread call QueryPerformanceCounter failed[%lu]\n", err);
            return 1;
        }

        if(now.QuadPart > time_to_wait.QuadPart) {
            /* call user function */
            handler(context);
            /* calcaute next time_to_wait */
            time_to_wait.QuadPart = now.QuadPart + interval.QuadPart;
        } else if(now.QuadPart + count_per_ms.QuadPart < time_to_wait.QuadPart ) {
            /* need to wait more than 1ms: use Sleep call */
            err = (DWORD)( (time_to_wait.QuadPart - now.QuadPart) / count_per_ms.QuadPart );
            /*
             * lmice_error_print("eal_timer_thread Sleep [%lu] ms\n", err);
             */
            Sleep(err);
        } else {
            /* drop remain thread time */
            Sleep(0);
        }
    }/* for-loop */

    return 0;
}
#else /*USE_GET_SYSTEM_TIME_AS_FILETIME */

DWORD WINAPI eal_timer_thread(LPVOID data)
{
    DWORD err = 0;
    int64_t time_to_wait;
    int64_t now;
    int64_t interval;
    int64_t count_per_ms;

    /* assign data context */
    lm_timer_ctx_t *ctx = (lm_timer_ctx_t*)data;
    void (*handler) (void*) = ctx->handler;
    volatile int64_t *quit_flag = ctx->quit_flag;
    void* context = ctx->context;
    interval = ctx->interval;

    /* clean data */
    eal_timer_free_context(ctx);

    count_per_ms = 10000LL ;

    GetSystemTimeAsFileTime((LPFILETIME)&time_to_wait);
    time_to_wait += interval;

    /* for-loop to work */
    for(;;) {
        if(*quit_flag == 1)
            break;

        GetSystemTimeAsFileTime((LPFILETIME)&now);

        if(now > time_to_wait) {
            /* call user function */
            handler(context);
            /* calcaute next time_to_wait */
            time_to_wait = now + interval;
        } else if(now + count_per_ms < time_to_wait ) {
            /* need to wait more than 1ms: use Sleep call */
            err = (DWORD)( (time_to_wait - now) / count_per_ms);
            /*
             * lmice_error_print("eal_timer_thread Sleep [%lu] ms\n", err);
             */
            Sleep(err);
        } else {
            Sleep(0);
        }
    }/* for-loop */

    return 0;
}

#endif /* USE_QUERY_PERFORMANCE_COUNTER */

typedef NTSTATUS (CALLBACK* NTSETTIMERRESOLUTION)
(
         IN ULONG DesiredTime,
         IN BOOLEAN SetResolution,
         OUT PULONG ActualTime
);
NTSETTIMERRESOLUTION NtSetTimerResolution;

typedef NTSTATUS (CALLBACK* NTQUERYTIMERRESOLUTION)
(
        OUT PULONG MaximumTime,
        OUT PULONG MinimumTime,
        OUT PULONG CurrentTime
);
NTQUERYTIMERRESOLUTION NtQueryTimerResolution;


void QueryTimerResolution(void){

    HMODULE hNtDll = LoadLibrary(TEXT("NtDll.dll"));
    if (hNtDll)
    {
            NtQueryTimerResolution = (NTQUERYTIMERRESOLUTION)GetProcAddress(hNtDll,"NtQueryTimerResolution");
            NtSetTimerResolution = (NTSETTIMERRESOLUTION)GetProcAddress(hNtDll,"NtSetTimerResolution");
            FreeLibrary(hNtDll);
    }
    if (NtQueryTimerResolution == NULL || NtSetTimerResolution  ==  NULL){
        printf("Search NtQueryTimerResolution function failed!\n");
        return ;
    }

    NTSTATUS nStatus;

    ULONG Min=0;
    ULONG Max=0;
    ULONG Cur=0;
    nStatus = NtQueryTimerResolution(&Max, &Min,&Cur);

    printf("NtQueryTimerResolution -> \nMax=%lu(100ns) Min=%lu(100ns) Cur=%lu(100ns)\n",Min,Max,Cur);

    //BOOL bSetResolution = TRUE;
    //ULONG nActualTime;
    //ULONG nDesiredTime = 20064;
    //nStatus =  NtSetTimerResolution (nDesiredTime, bSetResolution,&nActualTime);
}
#endif
