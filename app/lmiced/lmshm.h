#ifndef LMSHM_H
#define LMSHM_H

#include <stdint.h>
#include <errno.h>

#include "lmice_eal_common.h"
#include "lmice_eal_shm.h"

struct pubsub_shm_s{
    int type;
    uint64_t hval;
    lmice_shm_t shm;

};

typedef struct pubsub_shm_s pubsub_shm_t;

enum shmlist_e {
    SYMBOL_SHMSIZE = 8*1024*1024, /* 8MB */
    SHMLIST_COUNT = 256,
    SHM_PUB_TYPE = 4,   /*exclusive right */
    SHM_SUB_TYPE = 2,
    SHM_PUB_EAGAIN = EAGAIN,
};

struct shmlist_s {
    volatile int64_t lock;
    uint32_t count;
    pubsub_shm_t shmlist[SHMLIST_COUNT];
    struct shmlist_s *next;
};

typedef struct shmlist_s shmlist_t;

#ifdef __cplusplus
extern "C" {
#endif

shmlist_t* lm_shmlist_create();
int lm_shmlist_delete(shmlist_t* sl);

/* Pub-sub by hval */
int lm_shmlist_pub(shmlist_t* sl, uint64_t hval, pubsub_shm_t **pp);
int lm_shmlist_sub(shmlist_t* sl, uint64_t hval, pubsub_shm_t **pp);
int lm_shmlist_unpub(shmlist_t* sl, uint64_t hval);
int lm_shmlist_unsub(shmlist_t* sl, uint64_t hval);

/* Utilities */
int lm_shmlist_find_or_create(shmlist_t* sl, uint64_t hval, pubsub_shm_t** pps);
int lm_shmlist_find(shmlist_t* sl, uint64_t hval, pubsub_shm_t** pps);

#ifdef __cplusplus
}
#endif

#endif // LMSHM_H
