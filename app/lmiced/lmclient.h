#ifndef LMCLIENT_H
#define LMCLIENT_H

#include "lmshm.h"

#include "lmice_eal_common.h"
#include "lmice_eal_shm.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_event.h"

#include <stdint.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

enum client_e {
    CLIENT_SPCNT = 256,
    SYMBOL_LENGTH = 32,
    CLIENT_COUNT = 64,
    BOARD_SHMSIZE = 4*1024,
    CLIENT_ALIVE = 1,
    CLIENT_DEAD = 0,
};

struct symbol_shm_s {
    char symbol[SYMBOL_LENGTH];
    int type;
    pubsub_shm_t *ps; /* point to shmlist */
};

typedef struct symbol_shm_s symbol_shm_t;


struct client_s {
    int64_t lock;
    eal_tid_t tid;
    volatile int active;
    pid_t pid;
    char name[SYMBOL_LENGTH];
    lmice_event_t event;
    lmice_shm_t board;
    struct sockaddr_un addr;
    socklen_t addr_len;
    unsigned short count;
    symbol_shm_t symshm[CLIENT_SPCNT];

};
typedef struct client_s client_t;


struct clientlist_s {
    int64_t lock;
    uint32_t count;
    client_t cli[CLIENT_COUNT];
    struct clientlist_s * next;
};

typedef struct clientlist_s clientlist_t;

#ifdef __cplusplus
extern "C" {
#endif

clientlist_t* lm_clientlist_create();
int lm_clientlist_delete(clientlist_t* cl);

/* Register */
int lm_clientlist_register(clientlist_t* cl, eal_tid_t tid, pid_t pid, const char* name, struct sockaddr_un* addr, client_t** ppc);
int lm_clientlist_unregister(clientlist_t* cl, struct sockaddr_un *addr, client_t **ppc);

/* Maintain */
int lm_clientlist_maintain(clientlist_t* cl);

/* Utilities */
int lm_clientlist_find_or_create(clientlist_t* cl, struct sockaddr_un* addr, client_t** ppc);
int lm_clientlist_find(clientlist_t* sl, struct sockaddr_un* addr, client_t** ppc);
int lm_clientlist_find_pid(clientlist_t* sl, pid_t pid, client_t** ppc);
/* Publish/subscribe symbol */
int lm_client_pub(client_t* cli, pubsub_shm_t* ps, const char* symbol);
int lm_client_sub(client_t* cli, pubsub_shm_t* ps, const char* symbol);
int lm_client_unpub(client_t* cli, pubsub_shm_t* ps, const char* symbol);
int lm_client_unsub(client_t* cli, pubsub_shm_t* ps, const char* symbol);

int lm_client_find_or_create(client_t* cli, pubsub_shm_t* ps, const char* symbol, symbol_shm_t** pps);
int lm_client_find(client_t* cli, pubsub_shm_t* ps, const char* symbol, symbol_shm_t** pps);

#ifdef __cplusplus
}
#endif

#endif // LMCLIENT_H
