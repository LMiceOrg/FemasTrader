#ifndef LMICE_BLOOMFILTER_H
#define LMICE_BLOOMFILTER_H

#include <stdint.h>

/** BloomFilter
  *
  *
  *
*/
/* hash function size */
#define LMICE_EAL_BF_HASH_SIZE 16

/* bloom filter block unit size */
#define LMICE_EAL_BF_BLOCK_SIZE 4096

#ifdef __cplusplus
extern "C" {
#endif

/* hash function prototype */
typedef uint32_t (*eal_bf_hash_func)(const unsigned char* ctx, uint32_t size);
/* hash function list */
extern eal_bf_hash_func eal_bf_hash_list[LMICE_EAL_BF_HASH_SIZE] ;

/* hash value prototype */
typedef uint32_t eal_bf_hash_val[LMICE_EAL_BF_HASH_SIZE];

/* bloomfilter prototype */
struct lmice_eal_bloomfilter_s {
    uint64_t n;
    double f;
    uint32_t m;
    uint32_t k;
    void* addr;
};
typedef struct lmice_eal_bloomfilter_s lm_bloomfilter_t;

/**
 * @brief eal_bf_calculate: calculate bloomfilter m,k,real f
 * @param n: number of elements
 * @param f: false positive rate
 * @param m: bit width of array [bytes]
 * @param k: number of hash functions
 * @return 0:success, else calculate failed
 */
int eal_bf_calculate(uint64_t n, double f, uint32_t* m, uint32_t* k, double *rf);


/**
 * @brief eal_bf_value: calc hash value
 * @param ctx: content
 * @param size: length of content
 * @param val: bloomfilter value of the content
 * @return 0:success, else failed
 */
int eal_bf_value(const unsigned char* ctx, uint32_t size, eal_bf_hash_val* val);

int eal_bf_key(lm_bloomfilter_t* bf, const unsigned char* ctx, uint32_t size, uint32_t* val);

/**
 * @brief eal_bf_find: find bloomfilter
 * @param bf: bloomfilter
 * @param val: hash value
 * @return: 0:exist 1:not exist
 */
int eal_bf_find(lm_bloomfilter_t* bf, const uint32_t* val);


/**
 * @brief eal_bf_add: append val to bloomfilter
 * @param bf:bloomfilter
 * @param val: hash value
 * @return : 0 success else:faied
 */
int eal_bf_add(lm_bloomfilter_t* bf, const uint32_t* val);

#ifdef __cplusplus
}
#endif

#endif /** LMICE_BLOOMFILTER_H */

