#ifndef LMSHM_H
#define LMSHM_H

#include "lmice_eal_shm.h"

struct pubsub_shm_s{
    int type;
    lmice_shm_t shm;

};

typedef struct pubsub_shm_s pubsub_shm_t;

enum {
    SHMLIST_COUNT = 256,
};
struct shmlist_s {
    int64_t lock;
    uint32_t count;
    pubsub_shm_t shmlist[SHMLIST_COUNT];
    struct shmlist_s *next;
};

typedef struct shmlist_s shmlist_t;


#endif // LMSHM_H
