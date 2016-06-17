#ifndef TMP_RING_H
#define TMP_RING_H




struct lmice_mesg_desc_s {
    int64_t             timestamp;
    uint32_t            status;
    uint32_t            blk_id;
    uint64_t            offset;

};
typedef struct lmice_mesg_desc_s lm_mesg_desc_t;

/**
 * @brief The lmice_subscribe_instance_s struct 按对象订阅信息结构
 *
 */
struct lmice_subscribe_instance_s {
    volatile uint64_t   lock;   /* sync-purpose lock */
    uint64_t            id;     /* identity  */
    uint32_t            type;   /* structure type */

    uint32_t            blk_cnt;
    lmblk_info         block[MAX_MEMORY_BLOCK_COUNT];

    uint32_t            msg_sz;     /* message size(bytes) */
    uint32_t            msg_cnt;    /* message count */

    uint32_t            capacity;   /* 最大可读队列长度 */
    uint32_t            length;     /* 当前可读队列长度 */

    uint64_t            inst_id;    /* instance */
    uint64_t            type_id;    /* message type */

    lm_mesg_desc_t      msginfo[1]; /* 信息列表 */

    /* message content at the end */
};
typedef struct lmice_subscribe_instance_s lm_sub_inst_t;

struct lmice_subscribe_type_s {
    volatile uint64_t   lock;   /* sync-purpose lock */
    uint64_t            id;     /* identity  */
    uint32_t            type;   /* structure type */

    uint32_t            blk_cnt;
    lmblk_info         block[MAX_MEMORY_BLOCK_COUNT];

    uint32_t            inst_sz;    /* instance size(bytes) */
    uint32_t            inst_cnt;   /* message count */

    uint32_t            capacity;   /* 最大可读队列长度 */
    uint32_t            length;     /* 当前可读队列长度 */

    lm_bloomfilter_t    filter;     /* type filter */
    uint64_t            type_id;    /* message type */

    lm_mblock_t         inst_info[1]; /* 对象列表 */
};
typedef struct lmice_subscribe_type_s lm_sub_type_t;

struct lmice_subscribe_topic_s {
    volatile uint64_t   lock;   /* sync-purpose lock */
    uint64_t            id;     /* identity  */
    uint32_t            type;   /* structure type */

    uint32_t            blk_cnt;
    lmblk_info         block[MAX_MEMORY_BLOCK_COUNT];

    uint32_t            inst_sz;     /* instance size(bytes) */
    uint32_t            inst_cnt;    /* message count */

    uint32_t            capacity;   /* 最大可读队列长度 */
    uint32_t            length;     /* 当前可读队列长度 */

    lm_bloomfilter_t    filter;     /* topic filter */

    lm_mblock_t      inst_info[1]; /* 对象列表 */
};
typedef struct lmice_subscribe_topic_s lm_sub_topic_t;

/**
 * @brief lmspi_t 中间件实例
 */
typedef void* lmspi_t;

/**
 * 应用程序c      中间件i   平台e
 *                      1 首先启动，初始化平台，创建首个公共共享内存块（名称固定），存储资源描述列表
 *  2 启动，创建应用程序实例
 *              3 由应用程序调用，初始化中间件，创建首个信息容器内存块，管理应用申请的信息
*/
/** 平台层 lme_
 * 创建资源列表：  在初始化时创建资源列表
 * 创建内存块资源：创建资源信息，存储与资源列表(reslist)
 * 释放内存块资源：平台释放已创建的内存块
 * 释放资源列表：  在退出时释放资源列表，并同时清除其他未释放的内存块资源
 */
int lme_reslist_create(uint64_t id, uint32_t size, lme_mblist_info_t** list);
int lme_mblock_create(lme_mblist_info_t* list, uint64_t id, uint32_t size, shmfd_t* fd, addr_t* addr);
int lme_mblock_release(lme_mblist_info_t* list, uint64_t id);
int lme_reslist_release(lme_mblist_info_t** list);


/** 中间件层 lmi_
 * 创建容器描述列表：  在初始化时创建容器描述列表（ring）
 * 创建信息容器：
 * 释放信息容器:
 * 释放容器描述列表
  */
int lmi_ringlist_create(uint64_t id, uint32_t size, lmi_ringlist_t** list);
int lmi_ring_create(lmi_ringlist_t* list, uint64_t id, uint32_t size, uint32_t length, shmfd_t* fd, addr_t* addr);
int lmi_ring_release(lmi_ringlist_t* list, uint64_t id);
int lmi_ringlist_release(lmi_ringlist_t** list);

/** 应用程序层 lmc_
 * 创建会话实例
 * 注册订阅信息
 * 取消订阅
 * 注册发布信息
 * 取消发布
 * 删除会话实例
 */
int lmc_instance_create(uint64_t id, uint64_t session, lmspi_t* spi);
int lmc_register_subscribe_instance(lmspi_t *spi, uint64_t inst, uint64_t type, uint32_t length, uint32_t size, int* mesg);
int lmc_register_subscribe_type(lmspi_t *spi, uint64_t type, uint32_t length, uint32_t size, int* mesg);
int lmc_register_subscribe_topic(lmspi_t *spi, uint64_t topic, uint32_t length, int* mesg);
int lmc_unregister_subscribe(lmspi_t *spi, int mesg);
int lmc_register_publish(lmspi_t *spi, uint64_t inst, uint64_t type, int* mesg);
int lmc_unregister_publish(lmspi_t *spi, int mesg);
int lmc_instance_release(lmspi_t spi);

/**
 * @brief 环形容器
 * 自我描述，跨进程共享
 * 包括：权限管理,数据管理,读写操作
 * 写操作带有tick标识
 */


/* 数据块: 8字节对齐 */
struct lmice_ring_data_s {
    uint8_t                 data[8];    /* 内容块（8字节对齐） */
};
typedef struct lmice_ring_data_s lm_ring_data_t;
EAL_STRUCT_ALIGN(lm_ring_data_t);



/* 环形容器的可读数据块最大数量 */
#define MAX_READ_BLOCK_COUNT 32

/* 内存块信息 */
struct lmice_ring_memory_info_s {
    uint32_t flag;
    uint32_t size;
    uint64_t inst_id;
};
typedef struct lmice_ring_memory_info_s lm_ring_mem_info_t;

/* 内存块 */
struct lmice_ring_memory_s {
    lm_ring_data_t* address;
    uint32_t size;    /*内存大小 bytes */
    uint32_t count;     /* 可用数量 */
};
typedef struct lmice_ring_memory_s lm_ring_mem_t;

/* 数据块描述 */
struct lmice_ring_block_s {
    uint32_t sn;   /* 数据块编号 */
    uint32_t flag;     /* 数据块读标志 */
    lm_ring_data_t* ctx;    /* 数据块指针 */
};
typedef struct lmice_ring_block_s lm_ring_blk_t;

/** 环形容器:8字节对齐
 * 服务LMiced（aemon）拥有写权限，LMspi（Middleware）管理使用状态，Worker（Client）拥有读权限
    第一层 API:容器维护和初始化，管理可使用的内存块及数据块初始化
    第二层 API:管理可读数据块列表，管理可写数据块列表
    第三层 API:管理只读数据块队列，通过顺序、逆序访问数据块

    描述信息：lmice_ring_info_s
    资源信息: lmice_ring_res_s
*/

/* 环形容器描述信息
 * 位于共享内存当中
 */
struct lmice_ring_info_s {
    volatile int64_t lock; /* 全局锁 */
    uint64_t inst_id;   /*实例编号*/
    uint32_t reserved;  /*未使用 */

    uint32_t mmcount; /*内存块数量 */
    lm_ring_mem_info_t mminfo[MAX_MEMORY_BLOCK_COUNT]; /* 内存块信息列表 */

    uint32_t blkcount;                  /* 当前数据块数量 */
    uint32_t blksize;                   /* 数据块大小(bytes) */

    uint32_t capacity;                  /* 最大可读队列长度 */
    uint32_t length;                    /* 当前可读队列长度 */
    uint32_t wtblock;                   /* 当前可写数据块序号 */
};
typedef struct lmice_ring_info_s lm_ring_info_t;

/* 环形容器资源
 * 位于各进程地址中
 */
struct lmice_ring_resource_s {
    addr_t addr;
    evtfd_t efd;
    shmfd_t sfd;
    lm_ring_info_t* info;
    lm_ring_mem_t memlist[MAX_MEMORY_BLOCK_COUNT];  /* 内存块列表 */
    lm_ring_blk_t blklist[MAX_READ_BLOCK_COUNT];      /* 数据块列表 */
    lm_ring_data_t ctx[1];
};



#define CTX_POS (sizeof(lmice_ring_t) - 8)

enum lmice_ring_e {
    EAL_RING_WRITE = 0,
    EAL_RING_READ = 1,
    EAL_RING_EOK = 0,
    EAL_RING_ENOTFOUND = 2,
    EEAL_RING_OUTOFMEMORY,
    /* read flag */
    EAL_RING_BLKNOTUSED = 0xFFFFFFFF,
    EAL_RING_BLKREAD = 0
};

struct lmice_ring_ops;
typedef lm_ring_data_t* (*iterator)(struct lmice_ring_ops*);
struct lmice_ring_ops {
    lmice_ring_t* ring;
    iterator begin;
    iterator end;
    iterator rbegin;
    iterator rend;
};
typedef struct lmice_ring_ops lm_ring;



int lmice_ring_create(lmice_ring_t** ring, unsigned int data_size, unsigned int count);
int lmice_ring_destroy(lmice_ring_t* ring);

int eal_ring_acquire_right(lmice_ring_t* ring, int access, uint64_t id);
int eal_ring_release_right(lmice_ring_t* ring, int access, uint64_t id);

int lmice_ring_write(lmice_ring_t ring, void* data, unsigned int size);
int lmice_ring_read(lmice_ring_t ring, void** data, unsigned int* size);

typedef int rfd_t; /* ring descriptor type (Client) */
/* lmice_ring_t: ring structure (Middleware) */
/* lm_ring_info_t, lm_mem_info_t
 * : ring and memory information(Daemon)
 *
 * WorkerInstance(C) --API call-- LMspi(M) --IPC call-- LMiced(S)
 *
*/

/* API level 0: 容器维护与初始化

 *  从内存(blob-hval)上打开现存环形容器
 *  从内存(blob)上创建环形容器
 *  新建环形容器(C)
 *  关闭环形容器(C)
 *  增加内存块(C)
 *  修改队列长度(C)
 */
int eal_ring_open(uint64_t hval, rfd_t* id);
int eal_ring_create(void* mem_address, uint32_t mem_size, uint32_t queue_length, uint32_t block_size, lmice_ring_t** ppring);

int eal_ring_new(uint32_t mem_size, uint32_t queue_length, uint32_t block_size, rfd_t* id);

int eal_ring_close(rfd_t id);

int eal_ring_add_memory(rfd_t id, uint64_t hval);
int eal_ring_reserve(rfd_t id, uint32_t queue_length);

int eal_ring_get_memory_info(rfd_t id, uint32_t* mmcount, lm_ring_mem_info_t** mminfo);
int eal_ring_get_block_info(rfd_t id, uint32_t* blkcount, uint32_t* blksize);
int eal_ring_get_queue_info(rfd_t id, uint32_t* queue_length, uint32_t* queue_capacity);


#endif // TMP_RING_H

