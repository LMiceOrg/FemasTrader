#ifndef LMICE_EAL_HASH64_H
#define LMICE_EAL_HASH64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t eal_hash64_fnv1a(const void* data, uint32_t size);
uint64_t eal_hash64_more_fnv1a(const void* data, uint32_t size, uint64_t oldval);
uint32_t eal_hash32_fnv1a(const void* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /** LMICE_EAL_HASH64_H */
