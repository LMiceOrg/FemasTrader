#ifndef LMICE_EAL_ENDIAN_H
#define LMICE_EAL_ENDIAN_H

#include "lmice_eal_common.h"

forceinline int
eal_is_little_endian(void)
{
    unsigned short ed = 0x0001;
    return (char)ed;
}

forceinline int
eal_is_big_endian(void)
{
    unsigned short ed = 0x0100;

    return (char)ed;
}


#endif /** LMICE_EAL_ENDIAN_H */
