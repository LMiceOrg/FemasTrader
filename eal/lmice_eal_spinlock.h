#ifndef LMICE_EAL_SPINLOCK_H
#define LMICE_EAL_SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int eal_spin_trylock(volatile int64_t *lock);
int eal_spin_lock(volatile int64_t* lock);
int eal_spin_unlock(volatile int64_t *lock);

#ifdef __cplusplus
}
#endif
#endif /** LMICE_EAL_SPINLOCK_H */
