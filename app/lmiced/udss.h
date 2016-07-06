#ifndef UDSS_H
#define UDSS_H

#include "lmshm.h"
#include "lmclient.h"

#include "lmice_trace.h"
#include "lmice_eal_shm.h"
#include "lmice_eal_thread.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BOARD_NAME "LMiced V1.0"

/** SERVER BOARD
 *  |0                  ...                 1023|
 *   Server event list [pub, sym, reg]
 *  |1024               ...                 2047|
 *   Server publish list[PUBLIST_LENGTH]
 *  |2048               ...                 3071|
 *   Server symbol list[SYMLIST_LENGTH]
 *  |3072               ...                 4095|
 *   Server register list[REGLIST_LENGTH]
 */
enum symbol_client_e{
    CLIENT_SUBSYM = 2,
    CLIENT_PUBSYM = 4,
    CLIENT_BOARD = 4*1024,  /* 4KB for board */


    CLIENT_EVTPOS = 0,
    CLIENT_SUBPOS = 1024,
    SUBLIST_LENGTH = 64,

    MAINTAIN_PERIOD = 30,

    SERVER_EVTPOS = 0,
    SERVER_PUBPOS = 1024,
    PUBLIST_LENGTH = 64,

    SERVER_SYMPOS = 2048,
    SYMLIST_LENGTH = 18,
    SERVER_SYMEVT = 1*sizeof(sem_t),

    SERVER_REGPOS = 3072,
    REGLIST_LENGTH = 5,
    SERVER_REGEVT = 2*sizeof(sem_t),

};

/** CLIENT BOARD
 *  |0                  ...                 1023|
 *   Client event list[sub]
 *  |1024               ...                 2047|
 *   Client subscribe list[SUBLIST_LENGTH]
 */

enum lmice_spi_type_e {
    EM_LMICE_TRACE_TYPE=1,
	EMZ_LMICE_TRACEZ_BSON_TYPE,
    EM_LMICE_SUB_TYPE,
    EM_LMICE_UNSUB_TYPE,
    EM_LMICE_PUB_TYPE,
    EM_LMICE_UNPUB_TYPE,
    EM_LMICE_SEND_DATA,
    EM_LMICE_REGCLIENT_TYPE,
    EM_LMICE_UNREGCLIENT_TYPE,
};

typedef  struct {
    lmice_trace_info_t info;
    char symbol[32];
} lmice_sub_t;

typedef lmice_sub_t lmice_register_t;

typedef lmice_sub_t lmice_unsub_t;

typedef lmice_sub_t lmice_pub_t;

typedef lmice_pub_t lmice_unpub_t;

typedef struct sub_detail_s {
    uint32_t    pos;
    int32_t     size;
    uint64_t    hval;
} sub_detail_t;

typedef sub_detail_t pub_detail_t;

typedef struct {
    lmice_trace_info_t info;
    sub_detail_t sub;
} lmice_send_data_t;


typedef struct lmice_sub_data_s {
    int64_t lock;
    uint32_t padding;
    uint32_t count;
    sub_detail_t sub[1];
} lmice_sub_data_t;

typedef struct lmice_pub_data_s {
    int64_t lock;
    int32_t padding;
    int32_t count;
    pub_detail_t pub[1];
}lmice_pub_data_t;

typedef struct _lmice_symbol_detail_t {
    uint64_t  hval;     /* ID of symbol resource */
    pid_t     pid;      /* ID of client */
    int32_t   size;     /* data size in bytes */
    int32_t   count;    /* ring length */
    int32_t   type;     /* pub or sub */
    char symbol[SYMBOL_LENGTH]; /* resource name */
} lmice_symbol_detail_t;

typedef struct _lmice_symbol_data_t {
    int64_t lock;
    int32_t padding;
    int32_t count;
    lmice_symbol_detail_t sym[1];
}lmice_symbol_data_t;

typedef struct _lmice_register_detail_s {
    char symbol[SYMBOL_LENGTH];   /* name of client, for display purpose */
    struct sockaddr_un un;           /* key of client */
    int16_t len;
    int type;                   /* reg or unreg */
    eal_pid_t pid;
    eal_tid_t tid;
}lmice_register_detail_t;

typedef struct lmice_register_data_s {
    int64_t lock;
    int32_t padding;
    int32_t count;
    lmice_register_detail_t reg[1];
} lmice_register_data_t;

struct lmice_data_detail_s {
    int64_t lock;
    int32_t count;
    int32_t pos;
};
typedef struct lmice_data_detail_s lmice_data_detail_t;

struct uds_msg {
    int sock;
    struct sockaddr_un  remote_un;
    struct sockaddr_un  local_un;
    ssize_t size;
    char data[1024];
};

typedef struct uds_msg uds_msg;
#ifdef __cplusplus
extern "C" {
#endif


void create_uds_msg(void **ppmsg);
void delete_uds_msg(void **ppmsg);
int init_uds_server(const char *srv_name, uds_msg *msg);
int init_uds_client(const char *srv_name, uds_msg *msg);
int finit_uds_msg(uds_msg *msg);
int send_uds_msg(uds_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /** UDSS_H */

