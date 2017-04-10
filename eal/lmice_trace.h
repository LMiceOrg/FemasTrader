#ifndef LMICE_TRACE_H
#define LMICE_TRACE_H

#include "lmice_eal_common.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_time.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

enum lmice_trace_type_e
{
    lmice_trace_info        =0,
    lmice_trace_debug       =1,
    lmice_trace_warning     =2,
    lmice_trace_error       =3,
    lmice_trace_critical    =4,
    lmice_trace_none        =5,
    lmice_trace_time
};

typedef enum lmice_trace_type_e lmice_trace_type_t;

#if defined(_WIN32)
struct lmice_trace_name_s {
    lmice_trace_type_t type;
    WORD color;
    WORD padding;
    char name[16];

};
#define LMICE_TRACE_COLOR_TAG3(type) \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), lmice_trace_name[type].color); \
    printf(lmice_trace_name[type].name); \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), lmice_trace_name[lmice_trace_none].color);

#else
struct lmice_trace_name_s {
    lmice_trace_type_t type;
    char name[16];
    char color[16];
};

#define LMICE_TRACE_COLOR_TAG3(type) lmice_trace_name[type].color, lmice_trace_name[type].name, lmice_trace_name[lmice_trace_none].color

#endif
typedef struct lmice_trace_name_s lmice_trace_name_t;

extern const int lmice_trace_debug_mode;
extern lmice_trace_name_t lmice_trace_name[];

void eal_trace_color_print_per_thread(int type);

#define EAL_TRACE_COLOR_PRINT_THREAD(x)  eal_trace_color_print_per_thread(lmice_trace_##x);printf

#if defined(_WIN32)

/*
#define LMICE_TRACE_COLOR_PRINT(type, format, ...) do{ \
    char current_time[26];  \
    time_t tm;  \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug)  \
        break; \
    time(&tm);  \
    ctime_r(&tm, current_time); \
    current_time[24] = ' '; \
    printf(current_time); \
    LMICE_TRACE_COLOR_TAG3(type) \
    printf(":" format "\n", ##__VA_ARGS__); \
    }while (0);

#define LMICE_TRACE_COLOR_PRINT_PER_THREAD(type, format, ...) \
    do{ \
    int trace_ret_i;    \
    size_t trace_sz_i;  \
    time_t trace_tm_i;  \
    char trace_current_time_i[26];  \
    char trace_thread_name_i[32]; \
    if(lmice_trace_debug_mode == 0 && \
        type == lmice_trace_debug)  \
        break; \
    time(&trace_tm_i);  \
    ctime_r(&trace_tm_i, trace_current_time_i); \
    trace_current_time_i[24] = ' '; \
    trace_ret_i = pthread_getname_np(pthread_self(), trace_thread_name_i, 32);  \
    if(trace_ret_i == 0) {   \
        trace_sz_i = strlen(trace_thread_name_i);  \
        if(trace_sz_i == 0) trace_ret_i = -1;  \
        else trace_ret_i = 0;   \
    }   \
    printf(trace_current_time_i); \
    LMICE_TRACE_COLOR_TAG3(type); \
    if(trace_ret_i == 0) {   \
        printf(":[%d:%s]", getpid(), trace_thread_name_i); \
        printf(format , ##__VA_ARGS__); \
        printf("\n"); \
    } else {    \
        printf(":[%d:0x%d]", getpid(), pthread_self()); \
        printf(format , ##__VA_ARGS__); \
        printf("\n"); \
    }   \
    }while (0);
*/


#else /** Posix */


/*
#define LMICE_TRACE_COLOR_PRINT(type, format, ...) do{ \
    char current_time[26];  \
    time_t tm;  \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug)  \
        break; \
    time(&tm);  \
    ctime_r(&tm, current_time); \
    current_time[24] = ' '; \
    printf("%s%s%s%s:" format "\n", current_time, LMICE_TRACE_COLOR_TAG3(type), ##__VA_ARGS__); \
    }while (0);

#define LMICE_TRACE_COLOR_PRINT_PER_THREAD(type, format, args...) do{ \
    int _trace_ret;    \
    time_t _trace_tm;  \
    char _trace_current_time[26];  \
    char _trace_thread_name[32]; \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug)  \
        break; \
    time(&_trace_tm);  \
    ctime_r(&_trace_tm, _trace_current_time); \
    _trace_current_time[24] = ' '; \
    _trace_ret = pthread_getname_np(pthread_self(), _trace_thread_name, 32);  \
    if(_trace_ret == 0) {   \
        _trace_ret = strlen(_trace_thread_name);  \
        if(_trace_ret == 0) _trace_ret = -1;  \
        else _trace_ret = 0;   \
    }   \
    if(_trace_ret == 0)    \
        printf("%s%s%s%s:[%d:%s]" format "\n", \
            _trace_current_time, LMICE_TRACE_COLOR_TAG3(type), getpid(), _trace_thread_name, args); \
    else    \
        printf("%s%s%s%s:[%d:0x%p]" format "\n", \
            _trace_current_time, LMICE_TRACE_COLOR_TAG3(type), getpid(), pthread_self(), args); \
    }while (0);


#define LMICE_TRACE_PER_THREAD(env, type, format, ...) do{ \
    char current_time[26];  \
    time_t tm;  \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug)  \
        break; \
    time(&tm);  \
    ctime_r(&tm, current_time); \
    current_time[24] = ' '; \
    if(lmice_trace_debug_mode == 1) \
        fprintf((env)->logfd, \
        "%s**%s:thread[0x%x] -- %s[%d]"  format "\n", \
        current_time, lmice_trace_name[type].name, pthread_self(), __FILE__,__LINE__, ##__VA_ARGS__); \
    else    \
        fprintf((env)->logfd, \
        "%s**%s:thread[0x%x]"  format "\n", \
        current_time, lmice_trace_name[type].name, pthread_self(), ##__VA_ARGS__); \
    }while(0);


#define LMICE_TRACE(env, type, format, ...) do { \
    char current_time[26];  \
    time_t tm;  \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug)  \
        break; \
    time(&tm);  \
    ctime_r(&tm, current_time); \
    current_time[24] = ' '; \
    if(lmice_trace_debug_mode == 1) \
        fprintf((env)->logfd, \
        "%s**%s: -- %s[%d]"  format "\n", \
        current_time, lmice_trace_name[type].name,  __FILE__,__LINE__, ##__VA_ARGS__); \
    else    \
        fprintf((env)->logfd, \
        "%s**%s:"  format "\n", \
        current_time, lmice_trace_name[type].name,  ##__VA_ARGS__); \
    } while(0);

*/

#endif

#if defined(__linux__)
#define LMICE_TRACE_PER_THREAD(type, format, ...) do{ \
    char current_time[26];  \
    time_t tm;  \
    if(lmice_trace_debug_mode == 0 && type == lmice_trace_debug)  \
        break; \
    time(&tm);  \
    ctime_r(&tm, current_time); \
    current_time[24] = ' '; \
    FILE* fp = fopen("/var/log/lmiced.log", "a+");  \
    if(lmice_trace_debug_mode == 1) \
        fprintf(fp, \
        "%s--%s:[%d:0x%lx] -- %s[%d]"  format "\n", \
        current_time, lmice_trace_name[type].name, getpid(), pthread_self(), __FILE__,__LINE__, ##__VA_ARGS__); \
    else    \
        fprintf(fp, \
        "%s--%s:[%d:0x%lx]"  format "\n", \
        current_time, lmice_trace_name[type].name, getpid(), pthread_self(), ##__VA_ARGS__); \
    fclose(fp); \
    }while(0);

#define LMICE_TRACE_PER_THREAD2(info, format, ...) do{ \
    int log_level = info->loglevel;  \
    int size = strlen(format) + 32; \
    char *pformat = (char*)malloc(size);    \
    char current_time[26];  \
    ctime_r(&info->tm, current_time); \
    current_time[24] = ' '; \
    FILE* fp = fopen("/var/log/lmiced.log", "a+");  \
    memset(pformat, 0, size);   \
    strcat(pformat, "%s--%s:[%d:0x%lx]Log(%ld):");    \
    strcat(pformat, format);    \
    if(*(pformat+strlen(pformat)-1) != '\n') { \
        strcat(pformat, "\n");  \
    }   \
    if(log_level < lmice_trace_info || log_level > lmice_trace_none) \
        log_level = lmice_trace_info;   \
    fprintf(fp, \
            pformat,    \
            current_time, lmice_trace_name[info->loglevel].name, \
            info->pid, info->tid, info->systime, ##__VA_ARGS__); \
    fclose(fp); \
    }while(0);

#define LMICE_TRACE_PER_THREAD3(info, data, length) do{ \
		char bsonFilename[64];  \
		sprintf(bsonFilename, "/var/log/bson-%s-[%d-0x%lx]", info->model_name, info->pid, info->tid ); \
	    FILE* fp = fopen(bsonFilename, "ba+");  \
	    fwrite( data, length, 1, fp ); \
	    fclose(fp); \
    }while(0);

#endif

#define LMICE_TRACE_TYPE 1
typedef struct {
    int type;   /* type == 1 */
    time_t tm;
    int64_t systime;
    int loglevel;
    pid_t pid;
    int64_t tid;

} lmice_trace_info_t;


#define LMICE_TRACE_BSON_TYPE 2
typedef struct {
    int type;   /* type == 2 */
    time_t tm;
    int64_t systime;
    pid_t pid;
    int64_t tid;
	char model_name[32];

} lmice_trace_bson_info_t;



#define lmice_info_print        EAL_TRACE_COLOR_PRINT_THREAD(info)
#define lmice_debug_print       printf("%s:(%d)\n", __FILE__, __LINE__); EAL_TRACE_COLOR_PRINT_THREAD(debug)
#define lmice_warning_print     EAL_TRACE_COLOR_PRINT_THREAD(warning)
#define lmice_error_print       EAL_TRACE_COLOR_PRINT_THREAD(error)
#define lmice_critical_print    EAL_TRACE_COLOR_PRINT_THREAD(critical)


#define lmice_info_log(format,...) LMICE_TRACE_PER_THREAD(lmice_trace_info, format, ##__VA_ARGS__)
#define lmice_debug_log(format,...) LMICE_TRACE_PER_THREAD(lmice_trace_debug, format, ##__VA_ARGS__)
#define lmice_warning_log(format,...) LMICE_TRACE_PER_THREAD(lmice_trace_warning, format, ##__VA_ARGS__)
#define lmice_error_log(format,...) LMICE_TRACE_PER_THREAD(lmice_trace_error, format, ##__VA_ARGS__)
#define lmice_critical_log(format,...) LMICE_TRACE_PER_THREAD(lmice_trace_critical, format, ##__VA_ARGS__)

#define lmice_logging(info, format,...) LMICE_TRACE_PER_THREAD2(info, format, ##__VA_ARGS__)

#define lmice_logging_bson(info, data, length) LMICE_TRACE_PER_THREAD3(info, data, length)

#ifdef __cplusplus
}
#endif

#endif /** LMICE_TRACE_H */
