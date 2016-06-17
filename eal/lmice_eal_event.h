#ifndef LMICE_EAL_EVENT_H
#define LMICE_EAL_EVENT_H

#include "lmice_eal_common.h"
#include <stdint.h>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include <semaphore.h>
typedef sem_t* evtfd_t;
struct lmice_event_s
{
    evtfd_t fd;
    char name[32];
};
#elif defined(_WIN32)

typedef HANDLE evtfd_t;
#ifdef _W64
struct lmice_event_s
{
    evtfd_t fd;
    char name[32];
};
#else

struct lmice_event_s
{
    evtfd_t   fd;
    int32_t padding;
    char name[32];
};
#endif

#endif

typedef struct lmice_event_s lmice_event_t;

int eal_event_zero(lmice_event_t* e);

int eal_event_create(lmice_event_t *e);
int eal_event_destroy(lmice_event_t* e);
int eal_event_open(lmice_event_t* e);
int eal_event_awake(evtfd_t fd);
int eal_event_close(evtfd_t fd);

int eal_event_hash_name(uint64_t hval, char *name);

int eal_event_wait_one(evtfd_t fd);
int eal_event_wait_timed(evtfd_t fd, int millisec);

#endif /** LMICE_EAL_EVENT_H */

