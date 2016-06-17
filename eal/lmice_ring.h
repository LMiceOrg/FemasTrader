#ifndef LMICE_RING_H
#define LMICE_RING_H

#include <lmice_eal_align.h>
#include <lmice_eal_shm.h>

#include <stdint.h>

/* 容器的内存块最大数量 */
#define MAX_MEMORY_BLOCK_COUNT 16

/**
name: "LMiced shared memory block one"
hash: 36EE99391CA8AE12
*/

#define LMBLK_GID 0x36EE99391CA8AE12ULL
#define LMBLK_DEFAULT_SIZE (4096*16)
#define LMBLK_PAGE_SIZE 4096

/**
 *@brief 共享内存块:
 * MBlock
 *
 * OS isolate memory and other resources for security and general purpose,
 * here for performance purpose and in one publisher multiple subscribers perspective
 * SO shared memroy and spin-lock
 *
 * Identity(name, id, index):
 * user: a specified name [string]
 *
 * platform: fixed length id [uint64]
 * id = fnv1a(name), the unique value by FNV-1a hash function, in the public SP-Ring
 *
 * app: the index in the table [uint32]
 * index = the order in app's private SP-Ring
 */


/**
 *@brief 发布订阅环形状态容器:
 * SP-Ring
 *
 * The Subscribe-Publish Ring container(SP-Ring) presents the way of message sharing with many subscribers, publishers and one maintainer.
 *
 * The maintainer is unique in each computer(or VM).
 *
 * Subscribers and publishers are in different processes different computers(or VMs).
 *
 * Subers and Pubers use their SP-Ring to communicate with the maintainer, and vice versa.
 * Event be used for invoking others to work (one-to-one model).
 *
 * suber to mainter
 */

struct lmice_mblock_s {
    uint64_t id;    /* 标识符 */
    uint32_t size;  /* 大小(Bytes) */
#ifdef _WIN32
    uint32_t reserved1;
#endif
    shmfd_t  fd;    /* OS资源描述符 */
    addr_t   addr;  /* 映射的进程地址 */
};
typedef struct lmice_mblock_s lmblk_t;

struct lmice_mblock_info_s {
    uint64_t id;    /* unique identity */
    uint32_t size;  /* size of bytes */
    volatile int32_t rcount;  /* reference count */
};
typedef struct lmice_mblock_info_s lmblk_info;

/**
 * @brief The lmice_mesg_desc_status_enum_s enum 消息在队列中的状态
 */
enum lmice_spring_status_e {
    LMSPR_WRONLY = 0,
    LMSPR_RDWR = 1,
    LMSPR_RDONLY /* 2, 3, ... */
};

struct lmice_spring_status_s {
    int64_t             timestamp;
    volatile int32_t    status; /* value lmice_spring_status_e */
    uint32_t            flag;   /* reserved */
    uint32_t            blkindex;
    uint32_t            offset;

};
typedef struct lmice_spring_status_s lmspr_st;

struct lmice_spring_info_s {
    /* description */
    volatile int64_t    lock;   /* sync-purpose spinlock */
    uint64_t            id;     /* identity */
    uint32_t            rcount;   /* reference count */

    uint32_t            blkcount;
    lmblk_info          block[MAX_MEMORY_BLOCK_COUNT]; /* memory blocks*/

    uint32_t            blbcount;                  /* 当前数据块数量 */
    uint32_t            blbsize;                   /* 数据块大小(bytes) */
    uint32_t            capacity;                  /* 最大可读队列长度 */
    uint32_t            length;                    /* 当前可读队列长度 */
    /* status list (fixed size) */
    lmspr_st            st[1];                      /* capacity spr_sts */

    /* blobs of data(optional) */
};
typedef struct lmice_spring_info_s lmspr_info;

struct lmice_spring_s {
    uint32_t blkcount;
    lmblk_t  block[MAX_MEMORY_BLOCK_COUNT];
};
typedef struct lmice_spring_s lmspr_t;

int lmspr_readn(lmspr_t* spr, uint32_t n, lmspr_st **ppst, uint32_t* pn);
int lmspr_readp(lmspr_t* spr, int64_t ts, int64_t period, lmspr_st **ppst, uint32_t *pn);
int lmspr_release(lmspr_t* spr, lmspr_st* st, uint32_t n);
int lmspr_write(lmspr_t* spr, int64_t ts, void* blob, uint32_t size);

/** resource maintainer viewport */
/* Create a new Shared memory block */
int lmblk_create(lmblk_info*info, lmblk_t* blk);
int lmblk_open(lmblk_info* info, lmblk_t* blk);
int lmblk_close(lmblk_t* blk);
int lmblk_delete(lmblk_t* blk);

/* Create the SP-Ring container in the given memory block */
int lmspr_create(lmblk_info* info, uint64_t id, uint32_t blbcount, uint32_t blbsize,
                 uint32_t capacity, lmspr_t **pps);
int lmspr_delete(lmspr_t* ps);
int lmspr_open(lmblk_info* blk, lmspr_t ** pps);

#define lmspr_close(ps) do{ \
    lmspr_delete(ps); \
    } while(0);


/** worker(sub and pub) viewport */
/* initialize api */
int lmspr_init(int flag); /* initialize resource, thread only or not */
int lmspr_exit(); /* deallocate resources */
/* session api */
int lmspr_set_session_id(uint64_t session);
uint64_t lmspr_get_session_id();
/* register api */
int lmspr_register_publish_id(uint64_t instance, uint64_t type, uint32_t max_size);
int lmspr_register_subscribe_instance_id(uint64_t instance, uint64_t type, uint32_t capacity);
int lmspr_register_subscribe_type_id(uint64_t type, uint32_t capacity);
int lmspr_register_subscribe_topic_id(uint64_t topic, uint32_t capacity);
/* type api */
int lmspr_type_create_id(uint64_t type, uint32_t max_size);
/* topic api */
int lmspr_topic_create_id(uint64_t topic, const char* address, const char* port);
int lmspr_topic_add_instance_id(uint64_t topic, uint64_t instance, uint64_t type);
int lmspr_topic_add_type_id(uint64_t topic, uint64_t type);


#endif /** LMICE_RING_H */
