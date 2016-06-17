#ifndef LMICE_EAL_TIME_WIN_H
#define LMICE_EAL_TIME_WIN_H

#include "lmice_eal_common.h"
#include <time.h>
#include <stdint.h>

/**
    msdn.microsoft.com/en-us/magazine/cc163996.aspx
    Implement a Continuously Updating, High-Resolution Time Provider for Windows
*/
forceinline int eal_time_factor(uint64_t* factor) {
    *factor = 1;
    return 0;
}

int forceinline get_system_time(int64_t* t)
{
    LPFILETIME ft = (LPFILETIME)t;
    GetSystemTimeAsFileTime(ft);
    return 0;
}

void forceinline usleep(int usec){
    LARGE_INTEGER litmp;
    LONGLONG QPart1, QPart2;
    double dfMinus, dfFreq, dfTim;
    QueryPerformanceFrequency(&litmp);
    dfFreq = (double)litmp.QuadPart;

    QueryPerformanceCounter(&litmp);
    QPart1 = litmp.QuadPart;

    do {
        QueryPerformanceCounter(&litmp);
        QPart2 = litmp.QuadPart;

        dfMinus = (double)(QPart2-QPart1);
        dfTim = dfMinus / dfFreq;

    }while(dfTim<0.000001*usec);
}

int forceinline ctime_r(const time_t* tm, char* time)
{
    return ctime_s(time, 26, tm);
}

DWORD WINAPI eal_timer_thread(LPVOID param);

void QueryTimerResolution(void);

forceinline int eal_timer_create2(HANDLE *timer, lm_timer_ctx_t* ctx)
{
    *timer = CreateThread(NULL, 0 , eal_timer_thread, ctx, 0, NULL);
    if(*timer)
        return 0;
    return 1;
}

#define eal_timer_destroy2(ctx) \
    ctx->quit_flag = 1; \
    WaitForSingleObject(ctx->timer, INFINITE); \
    CloseHandle(ctx->timer);\
    ctx->timer = NULL;





#endif /** LMICE_EAL_TIME_WIN_H */

