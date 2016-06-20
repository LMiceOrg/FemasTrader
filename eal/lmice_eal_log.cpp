
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>

#include "lmice_trace.h"
#include "lmice_eal_log.h"

void EalLog::logging(const char* format, ...) {
    va_list argptr;

    time(&m_info.tm);
    get_system_time(&m_info.systime);
    memset(sid->data, 0, sizeof(sid->data));
    memcpy(sid->data, &m_info, sizeof(m_info) );
    sid->size = sizeof(m_info);

    va_start(argptr, format);
    sid->size += vsprintf(sid->data+sizeof(m_info), format, argptr);
    va_end(argptr);

    send_uds_msg(sid);
}


