#include "lmshm.h"

#include "lmice_eal_spinlock.h"

#include <stdlib.h>
#include <string.h>

shmlist_t * lm_shmlist_create()
{
    shmlist_t *ps;
    ps = malloc(sizeof(shmlist_t) );
    memset(ps, 0, sizeof(shmlist_t));

    return ps;
}


int lm_shmlist_delete(shmlist_t *sl)
{
    shmlist_t *next = sl;

    do {
        shmlist_t * cur = next;
        size_t i=0;
        for(i=0; i<cur->count; ++i) {
            pubsub_shm_t* ps = &cur->shmlist[i];
            lmice_shm_t *shm = &ps->shm;
            eal_shm_destroy(shm);
        }
        next = cur->next;
        free(cur);
    } while(next != NULL);
    return 0;
}


int lm_shmlist_pub(shmlist_t *sl, uint64_t hval, pubsub_shm_t **pp)
{
    int ret = 0;
    pubsub_shm_t* ps = NULL;

    /* Find or create new one */
    ret = lm_shmlist_find_or_create(sl, hval, &ps);

    /* Found the shm */
    if(ret ==0 && ps) {
        eal_spin_lock(&sl->lock);
        if(ps->type & SHM_PUB_TYPE) {
            ret = SHM_PUB_EAGAIN;
        } else {
            ps->type |= SHM_PUB_TYPE;
            if(pp != NULL) {
                *pp = ps;
            }
        }
        eal_spin_unlock(&sl->lock);
    }


    return ret;
}


int lm_shmlist_sub(shmlist_t *sl, uint64_t hval, pubsub_shm_t **pp)
{
    int ret = 0;
    pubsub_shm_t* ps = NULL;

    /* Find or create new one */
    ret = lm_shmlist_find_or_create(sl, hval, &ps);

    /* Found the shm */
    if(ret ==0 && ps) {
        eal_spin_lock(&sl->lock);
        ps->type |= SHM_SUB_TYPE;
        eal_spin_unlock(&sl->lock);
        if(pp != NULL) {
            *pp = ps;
        }
    }


    return ret;
}


int lm_shmlist_find_or_create(shmlist_t *sl, uint64_t hval, pubsub_shm_t **pps)
{
    int ret = 0;
    shmlist_t *ncur = NULL;
    pubsub_shm_t* nps = NULL;
    pubsub_shm_t * ps = NULL;
    shmlist_t* cur = sl;
    size_t i;

    eal_spin_lock(&sl->lock);
    do {
        /* Find an empty slot, may use it after */
        if(cur->count < SHMLIST_COUNT && nps == NULL) {
            ncur = cur;
            nps = &ncur->shmlist[ncur->count];
        }

        for(i=0; i<cur->count; ++i) {
            if(cur->shmlist[i].hval == hval) {
                ps = &cur->shmlist[i];
                /* Find it */
                break;
            }
        }
        /* Does not exist */
        if(ps == NULL && cur->next == NULL) {
            /* No empty slot, so create a new one */
            if(nps == NULL) {
                cur->next = lm_shmlist_create();
                ncur = cur->next;
                nps = ncur->shmlist;
            }

            /* Init shm */
            memset(nps, 0, sizeof(pubsub_shm_t));
            nps->hval = hval;
            eal_shm_hash_name(hval, nps->shm.name);
            nps->shm.size = SYMBOL_SHMSIZE;
            ret = eal_shm_create(&nps->shm);
            ++ncur->count;

            ps = nps;
            break;
        }
        cur = cur->next;
    } while(cur != NULL && ps == NULL);
    eal_spin_unlock(&sl->lock);

    *pps = ps;
    return ret;

}


int lm_shmlist_find(shmlist_t *sl, uint64_t hval, pubsub_shm_t **pps)
{
    pubsub_shm_t * ps = NULL;
    shmlist_t* cur = sl;
    size_t i;

    eal_spin_lock(&sl->lock);
    do {
        for(i=0; i<cur->count; ++i) {
            if(cur->shmlist[i].hval == hval) {
                ps = &cur->shmlist[i];
                /* Find it */
                break;
            }
        }

        cur = cur->next;
    }while(cur != NULL && ps == NULL);
    eal_spin_unlock(&sl->lock);

    *pps = ps;

    return 0;
}



int lm_shmlist_unpub(shmlist_t *sl, uint64_t hval)
{
    int ret = 0;
    pubsub_shm_t * ps = NULL;

    ret = lm_shmlist_find(sl, hval, &ps);

    if(ps) {
        ps->type &= SHM_SUB_TYPE;
    }

    return ret;
}


int lm_shmlist_unsub(shmlist_t *sl, uint64_t hval)
{
    (void)sl;
    (void)hval;
    /* Do nothing */
    return 0;
}
