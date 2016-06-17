#ifndef LMICE_EAL_KQUEUE_H
#define LMICE_EAL_KQUEUE_H

#include "lmice_eal_common.h"
#include <stdint.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

struct eal_kqueue_data_s {
    uint64_t id;        /* task identity */
    void* pdata;        /* user callback data */
    char buff[4096];    /* recv data */
};
typedef struct eal_kqueue_data_s eal_kqueue_data_t;

#define eal_kqueue_handle int

#define eal_kqueue_create_handle() kqueue()

#define eal_kqueue_t struct kevent64_s

#define eal_kqueue_add_read_event(pe, fd, id) EV_SET64(pe, fd, EVFILT_READ, EV_ADD, 0, 0, id, 0, 0 )
#endif /** LMICE_EAL_KQUEUE_H */
