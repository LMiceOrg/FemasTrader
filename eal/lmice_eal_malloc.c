#include "lmice_eal_malloc.h"


struct lmice_eal_allocators_s g_eal_allocators = {
    malloc,
    realloc,
    free
};


void eal_set_allocators(void *(*f_malloc)(size_t), void *(*f_realloc)(void *, size_t), void (*f_free)(void *))
{
    g_eal_allocators.f_malloc = f_malloc;
    g_eal_allocators.f_realloc = f_realloc;
    g_eal_allocators.f_free = f_free;
}
