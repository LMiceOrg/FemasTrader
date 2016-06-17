#ifndef LMICE_EAL_ATOMIC_H
#define LMICE_EAL_ATOMIC_H

#if defined(__GNUC__) && __GNUC__ > 3

#define eal_fetch_and_add32(ptr, value) __sync_fetch_and_add(ptr, value)
#define eal_fetch_and_add64(ptr, value) __sync_fetch_and_add(ptr, value)
#define eal_fetch_and_sub32(ptr, value) __sync_fetch_and_sub(ptr, value)
#define eal_fetch_and_sub64(ptr, value) __sync_fetch_and_sub(ptr, value)

#define eal_fetch_and_or32(ptr, value)  __sync_fetch_and_or(ptr, value)
#define eal_fetch_and_or64(ptr, value)  __sync_fetch_and_or(ptr, value)
#define eal_fetch_and_xor32(ptr, value) __sync_fetch_and_xor(ptr, value)
#define eal_fetch_and_xor64(ptr, value) __sync_fetch_and_xor(ptr, value)

#define eal_add_and_fetch32(ptr, value) __sync_add_and_fetch(ptr, value)
#define eal_add_and_fetch64(ptr, value) __sync_add_and_fetch(ptr, value)
#define eal_sub_and_fetch32(ptr, value) __sync_sub_and_fetch(ptr, value)
#define eal_sub_and_fetch64(ptr, value) __sync_sub_and_fetch(ptr, value)
#define eal_or_and_fetch32(ptr, value)  __sync_or_and_fetch(ptr, value)
#define eal_or_and_fetch64(ptr, value)  __sync_or_and_fetch(ptr, value)
#define eal_xor_and_fetch32(ptr, value) __sync_xor_and_fetch(ptr, value)
#define eal_xor_and_fetch64(ptr, value) __sync_xor_and_fetch(ptr, value)

#define eal_synchronize() __sync_synchronize()

#define eal_compare_and_swap32(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)
#define eal_compare_and_swap64(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)


#elif defined(_MSC_VER)
#define eal_fetch_and_add32(ptr, value) InterlockedExchangeAdd(ptr, value)
#define eal_fetch_and_add64(ptr, value) InterlockedExchangeAdd64(ptr, value)
#define eal_fetch_and_sub32(ptr, value) InterlockedExchangeAdd(ptr, -(value))
#define eal_fetch_and_sub64(ptr, value) InterlockedExchangeAdd64(ptr, -(value))

#define eal_compare_and_swap32(ptr, oldval, newval) InterlockedCompareExchange(ptr, newval, oldval)
#define eal_compare_and_swap64(ptr, oldval, newval) InterlockedCompareExchange64(ptr, newval, oldval)

#define eal_increment(pval) InterlockedIncrement(pval)
#define eal_decrement(pval) InterlockedDecrement(pval)
#define eal_xadd(pval, newval) InterlockedExchange(pval, newval)
#else
    #error(No atomic implementation!)
#endif


#endif /** LMICE_EAL_ATOMIC_H */
