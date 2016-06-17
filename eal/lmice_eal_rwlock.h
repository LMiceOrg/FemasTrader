#ifndef LMICE_EAL_RWLOCK_H
#define LMICE_EAL_RWLOCK_H

/** John Mellor-Crummey 和 Michael Scott 提出，
 * 详见"Scalable Reader-Writer Synchronization for Shared-Memory Multiprocessors"，
 */
#include "lmice_eal_common.h"
#include "lmice_eal_atomic.h"
#include "lmice_eal_time.h"
#include <errno.h>

union eal_read_write_ticket_u
{
    unsigned u;
    unsigned short us;
    __extension__ struct
    {
        unsigned char write;
        unsigned char read;
        unsigned char users;
    } s;
};

typedef union eal_read_write_ticket_u eal_rwticket_t;

forceinline void eal_rwlock_wlock(eal_rwticket_t *l)
{
    unsigned me = eal_xadd(&l->u, (1<<16));
    unsigned char val = me >> 16;

    while (val != l->s.write) eal_cpu_nop();
}

forceinline void eal_rwlock_wunlock(eal_rwticket_t *l)
{
    eal_rwticket_t t = *l;

    /*barrier();*/
    MemoryBarrier();

    t.s.write++;
    t.s.read++;

    *(unsigned short *) l = t.us;
}

forceinline int eal_rwlock_wtrylock(eal_rwticket_t *l)
{
    unsigned me = l->s.users;
    unsigned char menew = me + 1;
    unsigned read = l->s.read << 8;
    unsigned cmp = (me << 16) + read + me;
    unsigned cmpnew = (menew << 16) + read + me;

    if (eal_compare_and_swap32(&l->u, cmp, cmpnew) == cmp) return 0;

    return EBUSY;
}

forceinline void eal_rwlock_rlock(eal_rwticket_t *l)
{
    unsigned me = eal_fetch_and_add32(&l->u, (1<<16));
    unsigned char val = me >> 16;

    while (val != l->s.read) cpu_relax();
    l->s.read++;
}

forceinline void eal_rwlock_runlock(eal_rwticket_t *l)
{
    eal_increase(&l->s.write);
}

forceinline int eal_rwlock_rtrylock(eal_rwticket_t *l)
{
    unsigned me = l->s.users;
    unsigned write = l->s.write;
    unsigned char menew = me + 1;
    unsigned cmp = (me << 16) + (me << 8) + write;
    unsigned cmpnew = ((unsigned) menew << 16) + (menew << 8) + write;

    if(eal_compare_and_swap32(&l->u, cmp, cmpnew) == cmp) return 0;

    return EBUSY;
}

#endif /** LMICE_EAL_RWLOCK_H */

