#ifndef UDSS_H
#define UDSS_H

#include "lmshm.h"
#include "lmclient.h"

#include "lmice_trace.h"
#include "lmice_eal_shm.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BOARD_NAME "LMiced V1.0"

/** SERVER BOARD
 *  |0          ...     1023|
 *   Server event list [pub, reg, log]
 *  |1024       ...     2047|
 *   Server publish list
 *  |2048       ...     3071|
 *   Server symbol list
 *  |3072       ...     4095|
 *   Server logging list
 */
enum symbol_client_e{
    CLIENT_SUBSYM = 2,
    CLIENT_PUBSYM = 4,
    CLIENT_BOARD = 4*1024,  /* 8KB */


    CLIENT_SUBPOS = 1024,

    MAINTAIN_PERIOD = 30,
    SERVER_PUBPOS = 1024,
    SERVER_EVTPOS = 0,
    PUBLIST_LENGTH = 64,

    SERVER_SYMPOS = 2048,
    SYMLIST_LENGTH = 18,

    SERVER_LOGPOS = 3072
};

enum lmice_spi_type_e {
    EM_LMICE_TRACE_TYPE=1,
	EMZ_LMICE_TRACEZ_BSON_TYPE,
    EM_LMICE_SUB_TYPE,
    EM_LMICE_UNSUB_TYPE,
    EM_LMICE_PUB_TYPE,
    EM_LMICE_UNPUB_TYPE,
    EM_LMICE_SEND_DATA,
    EM_LMICE_REGCLIENT_TYPE,
};

typedef  struct {
    lmice_trace_info_t info;
    char symbol[32];
} lmice_sub_t;

typedef lmice_sub_t lmice_register_t;

typedef lmice_sub_t lmice_unsub_t;

typedef lmice_sub_t lmice_pub_t;

typedef lmice_pub_t lmice_unpub_t;

typedef struct {
    uint32_t    pos;
    int32_t     size;
    uint64_t    hval;
} sub_detail_t;

struct pub_detail_s {
    uint32_t    pos;
    int32_t     size;
    uint64_t    hval;
};
typedef struct pub_detail_s pub_detail_t;

typedef struct {
    lmice_trace_info_t info;
    sub_detail_t sub;
} lmice_send_data_t;


typedef struct {
    int64_t lock;
    uint32_t padding;
    uint32_t count;
    sub_detail_t sub[1];
} lmice_sub_data_t;

struct lmice_pub_data_s {
    int64_t lock;
    int32_t padding;
    int32_t count;
    pub_detail_t pub[1];
};
typedef struct lmice_pub_data_s lmice_pub_data_t;

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

