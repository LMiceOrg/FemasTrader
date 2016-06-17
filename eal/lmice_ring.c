#include "lmice_ring.h"
#include "lmice_eal_common.h"
#include "lmice_eal_spinlock.h"
#include "lmice_eal_atomic.h"
#include "lmice_eal_hash.h"

#include <stdlib.h>
#include <errno.h>


int lmblk_create(lmblk_info *info, lmblk_t* blk) {
    int ret = 0;
    lmice_shm_t st;

    eal_shm_zero(&st);

    eal_shm_hash_name(info->id, st.name);
    st.size = info->size;

    ret = eal_shm_create(&st);
    if(ret == 0) {
        blk->id = info->id;
        blk->size = info->size;
        blk->fd = st.fd;
        blk->addr = st.addr;

        info->rcount ++;
    }

    return ret;
}

int lmblk_open(lmblk_info* info, lmblk_t* blk) {
    int ret = 0;
    lmice_shm_t st;

    eal_shm_zero(&st);

    eal_shm_hash_name(info->id, st.name);
    st.size = info->size;

    ret = eal_shm_open_readwrite(&st);
    if(ret == 0) {
        blk->id = info->id;
        blk->size = info->size;
        blk->fd = st.fd;
        blk->addr = st.addr;

        info->rcount ++;
    }

    return ret;
}

int lmblk_close(lmblk_t* blk) {
    eal_shm_close(blk->fd, blk->addr);
    blk->fd = 0;
    blk->addr = 0;

    return 0;
}

int lmblk_delete(lmblk_t* blk) {
    lmice_shm_t st;
    st.addr = blk->addr;
    st.fd = blk->fd;
    eal_shm_destroy(&st);

    blk->fd = 0;
    blk->addr = 0;

    return 0;
}



int lmspr_create(lmblk_info *info, uint64_t id, uint32_t blbcount, uint32_t blbsize, uint32_t capacity, lmspr_t **pps)
{
    int ret = 0;
    lmblk_t* blk = NULL;
    lmspr_info *ps = NULL;
    uint32_t size;
    uint32_t i = 0;
    lmspr_t* spr = NULL;

    spr = (lmspr_t*)malloc( sizeof(lmspr_t) );
    spr->blkcount = 1;
    lmblk_create(info, spr->block);
    blk = spr->block;

    do {
        if(blbcount < capacity) {
            ret = ENOSPC;
        } else if(blk->size < sizeof(lmspr_t)+sizeof(lmspr_st)*blbcount ) {
            ret = E2BIG;
        } else {
            ps = (lmspr_info*)blk->addr;
            ps->lock = 0;
            ps->id = id;
            ps->rcount = 0;
            ps->blkcount = 1;
            ps->block[0].id = blk->id;
            ps->block[0].size = blk->size;
            ps->block[0].rcount = 1;
            ps->blbcount =blbcount;
            ps->blbsize = blbsize;
            ps->capacity = capacity;
            ps->length = 0;
            memset(ps->st, 0, sizeof(lmspr_st)*blbcount);
            *pps = spr;

            /* Blob size ==0, do not alloc blob memory */
            if(blbsize == 0) {
                break;
            }
            size = sizeof(lmspr_t)+ sizeof(lmspr_st)*blbcount;
            if(blk->size >= size + blbcount*blbsize) {
                /* Data in the same block */
                for(i=0; i<blbcount; ++i) {
                    ps->st[i].blkindex = 0;
                    ps->st[i].offset = size + i*blbsize;
                }
            } else {
                /* Data in Different block */
                id = eal_hash64_more_fnv1a(&size, sizeof(size), id);
                ps->blkcount ++;
                ps->block[1].id = id;
                ps->block[1].rcount = 0;
                ps->block[1].size = ((blbcount*blbsize+LMBLK_PAGE_SIZE-1)/LMBLK_PAGE_SIZE)*LMBLK_PAGE_SIZE;
                spr->blkcount ++;
                ret = lmblk_create(ps->block+1, spr->block + 1);

                for(i=0; i<blbcount; ++i) {
                    ps->st[i].blkindex = 1;
                    ps->st[i].offset = i*blbsize;
                }
            }
        }

    } while(0);

    if(ret != 0) {
        free(spr);
    }

    return ret;
}

int lmspr_open(lmblk_info *blk, lmspr_t ** pps) {

    uint32_t i = 0;
    lmspr_info *ps = NULL;
    lmspr_t* spr = NULL;

    spr = (lmspr_t*)malloc( sizeof(lmspr_t) );
    lmblk_open(blk, spr->block);
    ps = (lmspr_info*)spr->block[0].addr;

    spr->blkcount = ps->blkcount;
    for(i=1; i< spr->blkcount; ++i) {
        lmblk_open(ps->block+i, spr->block+i);
    }

    *pps = spr;

    return 0;
}

int lmspr_delete(lmspr_t* ps) {
    size_t i = 0;
    lmspr_info* info;

    info = (lmspr_info*)ps->block[0].addr;


    /* Decrement reference */
    for(i=0; i< info->blkcount; ++i) {
        eal_decrement(&(info->block[i].rcount));
    }

    /* Delete blocks */
    if(info->block[0].rcount == 0) {
        for(i=0; i<ps->blkcount; ++i) {
            lmblk_delete(ps->block+i);
        }
    } else {
        for(i=0; i<ps->blkcount; ++i) {
            lmblk_close(ps->block + i);
        }
    }

    /* free ps */
    free(ps);
    return 0;
}


int lmspr_readn(lmspr_t *spr, uint32_t n, lmspr_st **ppst, uint32_t *pn)
{
    uint32_t i = 0;
    lmspr_st *st = NULL;
    lmspr_info* info;
    info = (lmspr_info*)spr->block[0].addr;

    eal_spin_lock(&info->lock);

    do {
        *pn = info->length > n? n:info->length;
        if(*pn == 0) {
            break;
        }
        st = (lmspr_st*)malloc((*pn)*sizeof(lmspr_st));

        memcpy(st, info->st, (*pn)*sizeof(lmspr_st) );
        for(i=0; i< (*pn); ++i) {
            eal_increment(&(info->st[i].status));
        }
    } while(0);

    eal_spin_unlock(&info->lock);

    *ppst = st;

    return 0;
}


int lmspr_readp(lmspr_t *spr, int64_t ts, int64_t period, lmspr_st **ppst, uint32_t *pn)
{
    const uint32_t step = 8;
    uint32_t i = 0;
    uint32_t cnt = 4;
    uint32_t idx = 0;
    lmspr_st *st = NULL;
    lmspr_info* info = NULL;

    info = (lmspr_info*)spr->block[0].addr;

    cnt = step;
    st = (lmspr_st*)malloc(cnt*sizeof(lmspr_st));

    eal_spin_lock(&info->lock);

    for(i=0; i < info->length; ++i) {
        if(info->st[i].timestamp >= ts && info->st[i].timestamp < ts+period ) {
            eal_increment(&(info->st[i].status));
            if(idx+1 == cnt) {
                cnt += step;
                st = (lmspr_st*)realloc(st, cnt*sizeof(lmspr_st));
            }
            memcpy(st+idx, &(info->st[i]), sizeof(lmspr_st));
            ++idx;
        }
    }

    eal_spin_unlock(&info->lock);

    if(idx == 0) {
        free(st);
        st = NULL;
    }
    *ppst = st;
    *pn = idx;

    return 0;
}


int lmspr_release(lmspr_t *spr, lmspr_st *st, uint32_t n)
{
    uint32_t i = 0;
    uint32_t j = 0;
    lmspr_info* info = NULL;

    info = (lmspr_info*)spr->block[0].addr;

    do {
        /* Stop process when no element had been locked */
        if(n == 0) {
            break;
        }

        eal_spin_lock(&info->lock);
        for(i=0; i< info->blbcount; ++i) {
            if(info->st[i].timestamp == st->timestamp) {
                for(j=0; j<n; ++j) {
                    eal_decrement(&(info->st[i+j].status));
                }
                break;
            }
        }
        eal_spin_unlock(&info->lock);
    } while(0);

    return 0;
}


int lmspr_write(lmspr_t *spr, int64_t ts, void *blob, uint32_t size)
{
    uint32_t i = 0;
    lmspr_st st;

    lmspr_info* info = NULL;

    info = (lmspr_info*)spr->block[0].addr;

    eal_spin_lock(&info->lock);
    do {

        /* Stop write when ts <= the first element's timestamp */
        if(ts <= info->st[0].timestamp) {
            break;
        }

        /* Find a free element in list */
        for(i=info->length; i< info->blbcount; ++i) {
            if(info->st[i].status == LMSPR_WRONLY ||
                    info->st[i].status == LMSPR_RDWR) {
                memcpy(&st, &(info->st[i]), sizeof(lmspr_st) );
                st.timestamp = ts;
                st.status = LMSPR_RDWR;
                /* Move it to head */
                memmove(info->st +1, info->st, i*sizeof(lmspr_st));
                memcpy(info->st, &st, sizeof(lmspr_st));

                /* Update blob */
                if(size > 0 && size <= info->blbsize) {
                    memcpy((uint8_t*)(spr->block[st.blkindex].addr) + st.offset, blob, size );
                }
            }
        }

    } while(0);

    eal_spin_unlock(&info->lock);

    return 0;

}
