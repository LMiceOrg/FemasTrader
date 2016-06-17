#include "lmice_eal_hash.h"

/**
  http://www.isthe.com/chongo/tech/comp/fnv/index.html

Hash Size    Prime                       Offset
===========  =========================== =================================
32-bit       16777619                    2166136261
64-bit       1099511628211               14695981039346656037
128-bit      309485009821345068724781371 144066263297769815596495629667062367629
256-bit
    prime: 2^88 + 2^8 + 0x3b = 309485009821345068724781371
    offset: 100029257958052580907070968620625704837092796014241193945225284501741471925557
512-bit
    prime: 2^344 + 2^8 + 0x57 = 35835915874844867368919076489095108449946327955754392558399825615420669938882575126094039892345713852759
    offset: 9659303129496669498009435400716310466090418745672637896108374329434462657994582932197716438449813051892206539805784495328239340083876191928701583869517785
1024-bit
    prime: 2^680 + 2^8 + 0x8d = 5016456510113118655434598811035278955030765345404790744303017523831112055108147451509157692220295382716162651878526895249385292291816524375083746691371804094271873160484737966720260389217684476157468082573
    offset: 14197795064947621068722070641403218320880622795441933960878474914617582723252296732303
*/

uint64_t eal_hash64_fnv1a(const void* data, uint32_t len)
{
    uint32_t i=0;
    const unsigned char* dp = (const unsigned char*)data;
    uint64_t hval = 14695981039346656037ULL;
    for(i=0; i<len;++i)
    {
        hval ^= (uint64_t)*dp++;
#if defined(_DEBUG)
        hval *= 1099511628211ULL;
#else
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
                (hval << 7) + (hval << 8) + (hval << 40);
#endif
    }
    return hval;
}


uint32_t eal_hash32_fnv1a(const void *data, uint32_t len)
{
    uint32_t i=0;
    const unsigned char* dp = (const unsigned char*)data;
    uint32_t hval = 2166136261UL;
    /** xor the bottom with the current octet */
    for(i=0; i<len;++i)
    {
        hval ^= (uint32_t)*dp++;

        /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(_DEBUG)
        hval *= 16777619UL;
#else
        hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
    }
    return hval;
}



uint64_t eal_hash64_more_fnv1a(const void *data, uint32_t size, uint64_t oldval)
{
    uint32_t i=0;
    const unsigned char* dp = (const unsigned char*)data;
    uint64_t hval = oldval;
    for(i=0; i<size;++i)
    {
        hval ^= (uint64_t)*dp++;
#if defined(_DEBUG)
        hval *= 1099511628211ULL;
#else
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
                (hval << 7) + (hval << 8) + (hval << 40);
#endif
    }
    return hval;
}
