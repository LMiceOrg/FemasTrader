#ifndef LMICE_EAL_MALLOC_H
#define LMICE_EAL_MALLOC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

struct lmice_eal_allocators_s {
    void *(*f_malloc)(size_t);
    void *(*f_realloc)(void *, size_t);
    void (*f_free)(void *);
};

extern struct lmice_eal_allocators_s g_eal_allocators;


void eal_set_allocators(void* (*f_malloc) (size_t),
                              void* (*f_realloc) (void *, size_t),
                              void (*f_free) (void *) );

inline void *eal_malloc(size_t size)
{
    return g_eal_allocators.f_malloc(size);
}

inline void *eal_realloc(void *ptr, size_t size)
{
    return g_eal_allocators.f_realloc(ptr, size);
}

inline void eal_free(void *ptr)
{
    g_eal_allocators.f_free(ptr);
}

inline void *eal_calloc(size_t count, size_t size)
{
    void *ptr = g_eal_allocators.f_malloc(count * size);
    memset(ptr, 0, count * size);
    return ptr;
}

inline char *eal_strdup(const char *str)
{
    size_t length = strlen(str) + 1;
    char *rv = (char *)eal_malloc(length);
    memcpy(rv, str, length);
    return rv;
}

#ifdef __cplusplus
}
#endif

#endif /** LMICE_EAL_MALLOC_H */
