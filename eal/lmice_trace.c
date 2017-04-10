#include "lmice_trace.h"
#include "lmice_eal_time.h"
#include <stdint.h>
#include <stdio.h>

#if defined(_DEBUG)
const int lmice_trace_debug_mode = 1;
#else
const int lmice_trace_debug_mode = 0;
#endif

#if defined(_WIN32)

lmice_trace_name_t lmice_trace_name[] =
{
    {lmice_trace_info,     10,0,      "INFO" /* light_green*/},
    {lmice_trace_debug,    11,0,     "DEBUG" /* light_cyan */},
    {lmice_trace_warning,  14,0,   "WARNING" /*yellow*/},
    {lmice_trace_error,    12,0,     "ERROR" /*light_red*/},
    {lmice_trace_critical, 13,0,  "CRITICAL" /* light_purple*/},
    {lmice_trace_none,     7,0,       "NULL" /* white */}
};
#else

lmice_trace_name_t lmice_trace_name[] =
{
    {lmice_trace_info,      "INFO",     "\033[1;32m" /* light_green*/},
    {lmice_trace_debug,     "DEBUG",    "\033[1;36m" /* light_cyan */},
    {lmice_trace_warning,   "WARNING",  "\033[1;33m" /*yellow*/},
    {lmice_trace_error,     "ERROR",    "\033[1;31m" /*light_red*/},
    {lmice_trace_critical,  "CRITICAL", "\033[1;35m" /* light_purple*/},
    {lmice_trace_none,"NULL","\033[0m"},
    {lmice_trace_time, "TIME", "\033[1;33m" /*light_brown*/}
};

#endif

#define LMICE_TRACE_COLOR_TIME(tm) lmice_trace_name[lmice_trace_time].color, tm, lmice_trace_name[lmice_trace_none].color

#define EAL_TRACE_0() \
    int64_t _trace_stm; \
    int _trace_ret; \
    time_t _trace_tm;   \
    struct tm pt;  \
    char _trace_current_time[80];   \
    char _trace_thread_name[32]; \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug) \
        return; \
    memset(_trace_current_time, 0, sizeof(_trace_current_time));    \
    get_system_time(&_trace_stm);   \
    _trace_tm =_trace_stm / 10000000;   \
    gmtime_r(&_trace_tm, &pt);  \
    sprintf(_trace_current_time, "%4d-%02d-%02d %02d:%02d:%02d.%07lld", pt.tm_year+1900, pt.tm_mon+1, pt.tm_mday, pt.tm_hour, pt.tm_min, pt.tm_sec, _trace_stm%10000000);  \
    /*change newline to space */ \
    _trace_ret = pthread_getname_np(eal_gettid(), _trace_thread_name, 32); \
    if(_trace_ret == 0) { \
        if( strlen(_trace_thread_name) == 0) _trace_ret = -1; \
        else _trace_ret = 0; \
    }

#if defined(_WIN32)
#define EAL_TRACE_WIN32() \
    printf(_trace_current_time); \
    LMICE_TRACE_COLOR_TAG3(type); \
    if(_trace_ret == 0) {   \
        printf(":[%d:%s]", getpid(), _trace_thread_name); \
    } else {    \
        printf(":[%d:0x%llx]", getpid(), eal_gettid()); \
    }
#elif defined(__APPLE__) || defined(__linux__)
#define EAL_TRACE_UNIX() \
    if(_trace_ret == 0) {   \
        printf("%s%s%s %s%s%s:[%d:%s]",  \
            LMICE_TRACE_COLOR_TIME(_trace_current_time), LMICE_TRACE_COLOR_TAG3(type), getpid(), _trace_thread_name); \
    } else { \
        printf("%s%s%s %s%s%s:[%d:0x%lx]",    \
            LMICE_TRACE_COLOR_TIME(_trace_current_time), LMICE_TRACE_COLOR_TAG3(type), getpid(), (unsigned long)(void*)eal_gettid()); \
    }
#endif

void eal_trace_color_print_per_thread(int type)
{
    EAL_TRACE_0();
#if defined(_WIN32)
    EAL_TRACE_WIN32();
#else /* UNIX */
    EAL_TRACE_UNIX();
#endif
}

