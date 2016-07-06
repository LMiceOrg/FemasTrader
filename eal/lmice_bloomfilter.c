#include "lmice_bloomfilter.h"

#include <math.h>

uint32_t eal_bf_rshash (const unsigned char* str, uint32_t len) {
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    unsigned int i = 0;

    for(i=0; i<len; str++, i++) {
        hash = hash*a + (*str);
        a = a*b;
    }
    return hash;
}
/* End Of RS(Robert Sedgwicks) Hash Function */

uint32_t eal_bf_jshash (const unsigned char* str, uint32_t len) {
    uint32_t hash = 1315423911;
    unsigned int* ch;
    unsigned int i;
    for(i=0; i<len/4; ++i)
    {
        ch = (unsigned int*)str + i;
        hash ^= ((hash << 5) + *ch + (hash >> 2));
    }
    return hash;
}
/* End Of JS(Justin Sobel) Hash Function */


unsigned int eal_bf_pjwhash(const unsigned char* str, unsigned int len)
{
    const unsigned int BitsInUnsignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    const unsigned int ThreeQuarters = (unsigned int)((BitsInUnsignedInt  * 3) / 4);
    const unsigned int OneEighth = (unsigned int)(BitsInUnsignedInt / 8);
    const unsigned int HighBits = (unsigned int)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
    unsigned int hash = 0;
    unsigned int test = 0;
    unsigned int i = 0;

    for(i=0;i<len; str++, i++) {
        hash = (hash<<OneEighth) + (*str);
        if((test = hash & HighBits)  != 0) {
            hash = ((hash ^(test >> ThreeQuarters)) & (~HighBits));
        }
    }

    return hash;
}
/* End Of  P. J. Weinberger Hash Function */


unsigned int eal_bf_elfhash(const unsigned char* str, unsigned int len)
{
    unsigned int hash = 0;
    unsigned int x    = 0;
    unsigned int i    = 0;

    for(i = 0; i < len; str++, i++) {
        hash = (hash << 4) + (*str);
        if((x = hash & 0xF0000000L) != 0) {
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }
    return hash;
}
/* End Of ELF Hash Function */


unsigned int eal_bf_bkdrhash(const unsigned char* str, unsigned int len)
{
    unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
    unsigned int hash = 0;
    unsigned int i    = 0;

    for(i = 0; i < len; str++, i++)
    {
        hash = (hash * seed) + (*str);
    }

    return hash;
}
/* End Of BKDR Hash Function */


unsigned int eal_bf_sdbmhash(const unsigned char* str, unsigned int len)
{
    unsigned int hash = 0;
    unsigned int i    = 0;

    for(i = 0; i < len; str++, i++) {
        hash = (*str) + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}
/* End Of SDBM Hash Function */


unsigned int eal_bf_djbhash(const unsigned char* str, unsigned int len)
{
    unsigned int hash = 5381;
    unsigned int i    = 0;

    for(i = 0; i < len; str++, i++) {
        hash = ((hash << 5) + hash) + (*str);
    }

    return hash;
}
/* End Of DJB Hash Function */


unsigned int eal_bf_dekhash(const unsigned char* str, unsigned int len)
{
    unsigned int hash = len;
    unsigned int i    = 0;

    for(i = 0; i < len; str++, i++) {
        hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);
    }
    return hash;
}
/* End Of DEK Hash Function */


unsigned int eal_bf_bphash(const unsigned char* str, unsigned int len)
{
    unsigned int hash = 0;
    unsigned int i    = 0;
    for(i = 0; i < len; str++, i++) {
        hash = hash << 7 ^ (*str);
    }

    return hash;
}
/* End Of BP Hash Function */


unsigned int eal_bf_fnvhash(const unsigned char* str, unsigned int len)
{
    const unsigned int fnv_prime = 0x811C9DC5;
    unsigned int hash      = 0;
    unsigned int i         = 0;

    for(i = 0; i < len; str++, i++) {
        hash *= fnv_prime;
        hash ^= (*str);
    }

    return hash;
}
/* End Of FNV Hash Function */


unsigned int eal_bf_aphash(const unsigned char* str, unsigned int len)
{
    unsigned int hash = 0xAAAAAAAA;
    unsigned int i    = 0;

    for(i = 0; i < len; str++, i++) {
        hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*str) * (hash >> 3)) :
                                 (~((hash << 11) + (*str) ^ (hash >> 5)));
    }

    return hash;
}
/* End Of AP Hash Function */

unsigned int eal_bf_hflphash(const unsigned char *str,unsigned int len)
{
    unsigned int n=0;
    unsigned int i;
    char* b=(char *)&n;
    for(i=0;i<len;++i) {
        b[i%4]^=str[i];
    }
    return n%len;
}
/* End Of HFLP Hash Function*/

unsigned int eal_bf_hkhash(const unsigned char* str,unsigned int len)
{
    int result=0;
    unsigned int i=0;
    for (i=1;i<len;i++, str++)
        result += (*str)*3*i;
    if (result<0)
        result = -result;
    return result%len;
}
/*End Of HKHash Function */

unsigned int eal_bf_strhash(const unsigned char *str,unsigned int len)
{
    register unsigned int   h = 0;
    register const unsigned char *p = str;
    unsigned int i =0;
    for(i=0; i<len; ++i,++p) {
        h=31*h+*p;
    }

    return h;

}
/*End Of StrHash Function*/

unsigned int eal_bf_tianlHash(const unsigned char *str,unsigned int len)
{
    uint64_t urlHashValue=0;
    unsigned int i;
    unsigned char ucChar;
    if(len<=256)  {
        urlHashValue=16777216*(len-1);
    } else {
        urlHashValue = 42781900080;
    }
    if(len<=96) {
        for(i=1;i<=len;i++) {
            ucChar=str[i-1];
            if(ucChar<='Z'&&ucChar>='A')  {
                ucChar=ucChar+32;
            }
            urlHashValue+=(3*i*ucChar*ucChar+5*i*ucChar+7*i+11*ucChar)%1677216;
        }
    } else  {
        for(i=1;i<=96;i++)
        {
            ucChar=str[i+len-96-1];
            if(ucChar<='Z'&&ucChar>='A')
            {
                ucChar=ucChar+32;
            }
            urlHashValue+=(3*i*ucChar*ucChar+5*i*ucChar+7*i+11*ucChar)%1677216;
        }
    }
    return (uint32_t)urlHashValue;

}
/*End Of Tianl Hash Function*/

uint32_t eal_bf_murmur3hash(const unsigned char *key, uint32_t len) {
    static const uint32_t c1 = 0xcc9e2d51;
    static const uint32_t c2 = 0x1b873593;
    static const uint32_t r1 = 15;
    static const uint32_t r2 = 13;
    static const uint32_t m = 5;
    static const uint32_t n = 0xe6546b64;

    uint32_t seed = 0;

    uint32_t hash = seed;

    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *) key;
    int i;
    for (i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];

        k1 *= c1;
        k1 = (k1 << r1) | (k1 >> (32 - r1));
        k1 *= c2;
        hash ^= k1;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}
/* End of MurmurHash Hash Function */

/* Hash function declaration */
eal_bf_hash_func eal_bf_hash_list[LMICE_EAL_BF_HASH_SIZE]={eal_bf_rshash,
                                                           eal_bf_jshash,
                                                           eal_bf_pjwhash,
                                                           eal_bf_elfhash,
                                                           eal_bf_bkdrhash,
                                                           eal_bf_sdbmhash,
                                                           eal_bf_djbhash,
                                                           eal_bf_dekhash,
                                                           eal_bf_bphash,
                                                           eal_bf_fnvhash,
                                                           eal_bf_aphash,
                                                           eal_bf_hflphash,
                                                           eal_bf_hkhash,
                                                           eal_bf_strhash,
                                                           eal_bf_tianlHash,
                                                           eal_bf_murmur3hash};


int eal_bf_calculate(uint64_t n, double f, uint32_t *m, uint32_t *k, double* rf)
{
    int ret = 0;
    uint32_t bytes =(uint32_t)( n * log(f) / log(0.6185) /8.0);
    if(bytes == LMICE_EAL_BF_BLOCK_SIZE)
        *m = LMICE_EAL_BF_BLOCK_SIZE;
    else
        *m = (bytes / LMICE_EAL_BF_BLOCK_SIZE + 1) * LMICE_EAL_BF_BLOCK_SIZE;
    *k = (uint32_t) (-log(f)/log(2) );

    if(*k > 16) {
        *k = 16;
        ret = 1;
    }

    if(*k == 0) {
        *k = 4;
    }

    *rf = pow( (1-exp(-1.0*(*k)*(double)n/(*m * 8) ) ), *k);

    return ret;
}


int eal_bf_value(const unsigned char *ctx, uint32_t size, eal_bf_hash_val *val)
{
    int i=0;
    for(i=0; i< LMICE_EAL_BF_HASH_SIZE; ++i)
        (*val)[i] = eal_bf_hash_list[i](ctx, size);
    return 0;
}


int eal_bf_find(lm_bloomfilter_t *bf, const eal_bf_hash_val *val)
{
    int ret = 0;
    uint32_t i=0;
    uint32_t n;
    uint8_t value;
    uint8_t* vector =(uint8_t*)bf->addr;
    for(i=0; i < bf->k; ++i) {
        n = (*val[i]) % (bf->m * 8);
        value = (vector[n/8] >>(n%8) ) & 1;
        if(value == 0) {
            ret = 1;
            break;
        }
    }
    return ret;
}


int eal_bf_add(lm_bloomfilter_t *bf, const eal_bf_hash_val *val)
{
    uint32_t i=0;
    uint32_t n;
    uint8_t* vector =(uint8_t*)bf->addr;
    for(i=0; i < bf->k; ++i) {
        n = (*val[i]) % (bf->m  * 8);
        vector[n/8] |= (1<<n%8);
    }

    return 0;
}


int eal_bf_key(lm_bloomfilter_t *bf, const unsigned char *ctx, uint32_t size, eal_bf_hash_val *val)
{
    int i=0;
    for(i=0; i< bf->k; ++i)
        (*val)[i] = eal_bf_hash_list[i](ctx, size);
    return 0;
}
