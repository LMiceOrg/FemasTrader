#ifndef UDSS_H
#define UDSS_H

#include "lmshm.h"
#include "lmclient.h"

#include "lmice_trace.h"
#include "lmice_eal_shm.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

enum symbol_client_e{
    CLIENT_SUBSYM = 2,
    CLIENT_PUBSYM = 4,
    CLIENT_BOARD = 4*1024,  /* 8KB */


    CLIENT_SUBPOS = 1024,
    MAINTAIN_PERIOD = 30
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
    uint32_t pos;
    uint32_t size;
    char symbol[SYMBOL_LENGTH];
} sub_detail_t;

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

