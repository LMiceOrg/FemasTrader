#include "udss.h"

#include "lmshm.h"
#include "lmclient.h"

#include "lmice_trace.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_shm.h"
#include "lmice_eal_hash.h"
#include "lmice_eal_event.h"
#include "lmice_eal_spinlock.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/epoll.h>


#include <sys/socket.h>
#include <sys/un.h>

#include <string.h>

#define PID_FILE "/var/run/lmiced.pid"
#define SOCK_FILE "/var/run/lmiced.socket"

#define BOARD_NAME "LMiced V1.0"

volatile int g_quit_flag = 0;

#define MAXEVENTS 64




/** sub list */




struct server_s {
    int64_t lock;
    int fd; /*for .pid file lock */
    uds_msg* pmsg;
    lmice_shm_t board;
//    struct sockaddr_un addr;
//    uint32_t count;
//    client_t *clients[CLIENT_COUNT];
//    char symbol[SYMBOL_LENGTH];
//    lmice_event_t event;
    clientlist_t *clilist;
    shmlist_t * shmlist;
//    struct server_s* next;

};
typedef struct server_s server_t;


/** Global publish/subscribe client list */
server_t g_server;


//forceinline int find_client(server_t* cur, struct sockaddr_un* addr, client_t** client) {
//    size_t i;
//    do {
//        for(i=0; i<cur->count; ++i) {
//            client_t* cli = cur->clients[i];
//            if( memcmp(&cli->addr, addr, cli->addr_len) == 0) {
//                *client = cli;
//                return 0;
//            }
//        }
//        cur = cur->next;
//    } while(cur != NULL);
//    *client = NULL;
//    return -1;
//}

//forceinline int append_client(server_t* cur, client_t** client) {
//    do {
//        if(cur->count < CLIENT_COUNT) {
//            client_t* cli = (client_t*)malloc(sizeof(client_t));
//            cur->clients[cur->count] = cli;
//            cur->count ++;
//            *client = cli;
//            return 0;
//        } else {
//            server_t* ser = (server_t*)malloc(sizeof(server_t));
//            memset(ser, 0, sizeof(server_t));
//            cur->next = ser;
//        }
//        cur = cur->next;
//    } while(cur != NULL);
//    *client = NULL;
//    return -1;
//}

//forceinline int init_board(client_t* client, struct sockaddr_un *addr, socklen_t addr_len) {
//    int ret;
//    uint64_t hval;
//    hval = eal_hash64_fnv1a(addr->sun_path, strlen(addr->sun_path));
//    eal_shm_hash_name(hval, client->board.name);
//    client->board.size = CLIENT_BOARD;
//    ret = eal_shm_create(&client->board);
//    if(ret != 0) {
//        lmice_error_log("EAL create board[%s] failed as[%d]\n", addr->sun_path, ret);
//    }
//    return ret;
//}

//forceinline int init_client(client_t* client, const lmice_trace_info_t* info, struct sockaddr_un *addr, socklen_t addr_len) {
//    int ret;
//    uint64_t hval;
//    memcpy(&client->addr, addr, addr_len);
//    client->pid = info->pid;
//    client->tid = info->tid;
//    client->addr_len = addr_len;
//    client->count = 0;
//    client->lock = 0;
//    hval = eal_hash64_fnv1a(addr->sun_path, strlen(addr->sun_path));
//    eal_event_hash_name(hval, client->event.name);
//    ret = eal_event_create(&client->event);
//    if(ret != 0) {
//        lmice_error_log("EAL create event[%s] failed as[%d]\n", addr->sun_path, ret);
//    }
//    ret |= init_board(client, addr, addr_len);
//    return ret;
//}



#define GET_SHMNAME(symbol, sym_len, hval, name) do {  \
    hval = eal_hash64_fnv1a(symbol, sym_len);   \
    eal_shm_hash_name(hval, name);  \
    } while(0)

//forceinline int append_symbol(client_t* client, const char* symbol, size_t sym_len, int type) {
//    int ret = 1;
//    size_t i;
//    uint64_t hval = 0;
//    char name[32] = {0};
//    GET_SHMNAME(symbol, sym_len, hval, name);

//    for(i=0; i< client->count; ++i) {
//        pubsub_shm_t* ps = &client->resshm[i];
//        lmice_shm_t* shm = &ps->shm;
//        if(memcmp(shm->name, name, 32) == 0 && ps->type == type) {
//            /* Already appended! */
//            lmice_critical_log("Client[%s] already append the symbol[%s] for [%d]\n", client->addr.sun_path, symbol, type);
//            ret = 0;
//            break;
//        }
//    }
//    if(ret == 1) {
//        /* Find nothing, so create new symbol */
//        if(client->count < SYMBOL_LENGTH) {
//            pubsub_shm_t* ps = &client->resshm[client->count];
//            lmice_shm_t* shm = &ps->shm;
//            ps->type = type;
//            ++client->count;
//            eal_shm_zero(shm);
//            eal_shm_hash_name(hval, shm->name);
//            shm->size = SYMBOL_SHMSIZE;
//            ret = eal_shm_create_or_open(shm);
//            if(ret != 0) {
//                lmice_error_log("Create shm[%s] failed as [%d].", name, ret);
//            }
//        } else {
//            lmice_error_log("Client[%s] symbol list is full, can't append symbol[%s]\n", client->addr.sun_path, symbol);
//        }
//    }
//    return ret;
//}

//forceinline int remove_symbol(client_t *client, const char* symbol, size_t sym_len, int type) {
//    int ret;
//    size_t i;
//    uint64_t hval = 0;
//    char name[32] = {0};
//    GET_SHMNAME(symbol, sym_len, hval, name);
//    for(i=0; i<client->count; ++i) {
//        pubsub_shm_t* ps = &client->resshm[i];
//        lmice_shm_t* shm = &ps->shm;
//        if(memcmp(shm->name, name, 32) == 0 && ps->type == type) {
//            ret = eal_shm_destroy(shm);
//            if(ret != 0) {
//                lmice_error_log("remove symbol[%s] [%d] failed as [%d].", shm->name, ps->type, ret);
//            }
//            memmove(ps, ps+1, (client->count-i-1)*sizeof(pubsub_shm_t) );
//            --client->count;
//            ret = 0;
//            break;
//        }
//    }

//    return ret;
//}

//forceinline int removeall_symbol(client_t* client) {
//    int ret = 0;
//    size_t i;
//    for(i=0; i<client->count; ++i) {
//        pubsub_shm_t* ps = &client->resshm[i];
//        lmice_shm_t* shm = &ps->shm;
//        ret = eal_shm_destroy(shm);
//        if(ret != 0) {
//            lmice_error_log("remove shm[%s] failed as [%d].", shm->name, ret);
//        }
//    }
//    client->count = 0;
//    return ret;
//}

//forceinline int finit_client(client_t* client) {
//    int ret = 0;
//    ret |= removeall_symbol(client);
//    ret |= eal_event_destroy(&client->event);
//    ret |= eal_shm_destroy(&client->board);
//    if(ret != 0) {
//        lmice_error_log("EAL finit client[%s] failed as[%d]\n", client->addr.sun_path, ret);
//    }
//    return ret;
//}

//forceinline int maint_client(server_t* ser) {
//    int ret;
//    int err;
//    size_t i;
//    clientlist_t *cur = ser->clilist;
//    do {
//        if(cur->count == 0)
//            break;
//        for(i= cur->count -1; i>=0; --i) {
//            client_t* cli = &cur->cli[i];
//            ret = kill(cli->pid, 0);
//            if(ret != 0) {
//                err = errno;
//                lmice_critical_log("process[%u] open failed[%d]\n", cli->pid, err);
//                lm_clientlist_unregister(cur, &cli->addr);
//                memmove(cli, cur->clients[i+1], sizeof(void*) * (cur->count-i-1) );
//                --cur->count;
//                continue;
//            }
//        }
//        cur = cur->next;
//    } while(cur != NULL);
//    return 0;
//}

//#define CLI_APPENDSYMBOL(info, ser,cli, msg, addr_len, sym, sym_len, type) do { \
//    int ret = 0;    \
//    /* Find or create client */ \
//    ret = find_client(ser, &msg.remote_un, &cli);   \
//    if(ret != 0) {  \
//        /* Don't found, so append new client */ \
//        ret = append_client(ser, &cli); \
//        if(ret != 0) {  \
//            lmice_error_log("Create client failed[%d]\n", ret);  \
//        } else {                                                \
//            init_client(cli, info, &msg.remote_un, addr_len);          \
//        }                                                       \
//    }                                                           \
//    /* Append symbol */ \
//    append_symbol(cli, sym, sym_len, type); \
//    } while(0)

//#define CLI_REMOVESYMBOL(ser, cli, msg, sym, sym_len, type) do{  \
//    int ret =0; \
//    /* Find client */   \
//    ret = find_client(ser, &msg.remote_un, &cli);   \
//    if(ret == 0) {  \
//        /* Remove symbol */ \
//        remove_symbol(cli, sym, sym_len, type); \
//    }   \
//    } while(0)


static int init_daemon();
int init_epoll(int sfd);

/* write hugepage */
int write_msg(const char* msg, int size);

void signal_handler(int sig) {
    if(sig == SIGTERM)
        g_quit_flag = 1;
}

int main(int argc, char* argv[]) {
    uint64_t hval;
    int rt;

    (void)argc;
    (void)argv;

    memset(&g_server, 0, sizeof(g_server));

    lmice_info_print("LMice server launching...\n");
    /* Init Daemon */
    g_server.fd = init_daemon();
    if(g_server.fd < 0) {
        lmice_error_log("Daemon init failed\n");
        return g_server.fd;
    }

    /* Init pub/sub list */
    g_server.shmlist = lm_shmlist_create();
    g_server.clilist = lm_clientlist_create();

    /* Create Hugepage SharedMemory */
    eal_shm_zero(&g_server.board);
    hval = eal_hash64_fnv1a(BOARD_NAME, sizeof(BOARD_NAME)-1);
    eal_shm_hash_name(hval, g_server.board.name);
    g_server.board.size = CLIENT_BOARD; /* 4K bytes*/
    rt = eal_shm_create(&g_server.board);
    if(rt != 0) {
        lmice_error_log("Shm board init failed\n");
        return rt;
    }

    /* Listen and wait to end */
    /*signal(SIGCHLD,SIG_IGN);  ignore child */
    /* signal(SIGTSTP,SIG_IGN);  ignore tty signals */
    signal(SIGTERM,signal_handler); /* catch kill signal */

    lmice_info_log("LMice server running.");

    create_uds_msg((void**)&g_server.pmsg);
    init_uds_server(SOCK_FILE, g_server.pmsg);

    init_epoll(g_server.pmsg->sock);

    /* Stop and clean resources */
    lmice_info_log("LMice server stopping...\n");
    rt=0;
    rt |= finit_uds_msg(g_server.pmsg);
    rt |= eal_shm_destroy(&g_server.board);
    rt |= lm_clientlist_delete(g_server.clilist);
    rt |= lm_shmlist_delete(g_server.shmlist);
    rt |= close(g_server.fd);
    rt |= unlink(PID_FILE);
    lmice_info_log("LMice server stopped [%d].\n", rt);

    return rt;
}


int init_daemon() {
    struct flock lock;
    eal_pid_t pid;
    char buf[32] = {0};
    ssize_t sz;

    if( daemon(0, 1) != 0) {
        return -2;
    }

    int fd = open(PID_FILE, O_CREAT|O_RDWR, 0644);
    if (fd < 0)
    {
        return -1;
    }

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl(fd, F_SETLK, &lock) != 0)
    {
        return -1;
    }

    pid = getpid();
    sprintf(buf, "%d\n", pid);
    sz = write(fd, buf, strlen(buf));
    if(sz != strlen(buf)) {
        return -1;
    }

    return fd;
}


int init_epoll(int sfd) {
    int ret;
    int s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;

    efd = epoll_create1 (0);
    if (efd == -1) {
        lmice_error_log("Epoll create failed.");
        return -1;
    }

    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1)
    {
        lmice_error_log("Epoll ctl add failed.");
        return -1;
    }

    /* Buffer where events are returned */
    events = (struct epoll_event*)malloc(MAXEVENTS * sizeof event);

    /* The event loop */
    time_t last = time(NULL);
    while (1)
    {
        int n, i;
        time_t now;

        /* wait socket, timeout(30ms) */
        n = epoll_wait (efd, events, MAXEVENTS, 500);

        if(g_quit_flag == 1) {
            lmice_info_log("Epoll wait quit.");
            break;
        }
        if (n<0) {
            lmice_error_log("Epoll wait error.");
            break;
        } else if(n == 0) {
            /*lmice_info_log("Epoll timeout.");*/
        }

        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN)))
            {
                /* An error has occured on this fd, or the socket is not
                     ready for reading (why were we notified then?) */
                lmice_error_log("Epoll error occured.");
                close(events[i].data.fd);
                continue;
            } else {
                /* We have data on the fd waiting to be read. Read and
                     display it. We must read whatever data is available
                     completely, as we are running in edge-triggered mode
                     and won't get a notification again for the same
                     data. */
                ssize_t count;
                uds_msg msg = {0};
                socklen_t addr_len=sizeof(msg.remote_un);
                msg.size = recvfrom(events[i].data.fd, msg.data, sizeof msg.data, 0, (struct sockaddr*)&(msg.remote_un), &addr_len);
                if (msg.size == -1)
                {
                    /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                    if (errno != EAGAIN)
                    {
                        lmice_error_log("Read(socket) error occured.");
                    }
                } else {
                    lmice_info_log("Read(socket) from %s[%ld] done.", msg.remote_un.sun_path, msg.size);
                    switch((int)(*msg.data)) {
                    case EM_LMICE_TRACE_TYPE:
                    {
                        const lmice_trace_info_t* info = (const lmice_trace_info_t*)msg.data;
                        const char* data = (const char*)(msg.data + sizeof(lmice_trace_info_t));
                        lmice_logging(info, data);
                        break;
                    }
                    case EMZ_LMICE_TRACEZ_BSON_TYPE:
                    {
                        const lmice_trace_bson_info_t* info = (const lmice_trace_bson_info_t*)msg.data;
                        const char* data = (const char*)(msg.data + sizeof(lmice_trace_bson_info_t));
                        unsigned int length = *(const unsigned int*)data;
                        //bson data length is default little endia
                        //memcpy(&length, data, sizeof(length));
                        lmice_logging_bson(info, data, length);
                        break;
                    }
                    case EM_LMICE_REGCLIENT_TYPE:
                    {
                        const lmice_register_t* reg = (const lmice_register_t*)msg.data;
                        const lmice_trace_info_t* info = &reg->info;
                        server_t* ser=&g_server;
                        client_t* cli = NULL;
                        /* Register client */
                        int ret = 0;
                        lmice_info_log("Client[%s] register in [%s]...\n", reg->symbol, msg.remote_un.sun_path);
                        /* Find or create client */
                        ret = lm_clientlist_register(ser->clilist, info->tid, info->pid, reg->symbol, &msg.remote_un);

                        /* Send state to client */
                        if(ret == 0) {
                            lm_clientlist_find(ser->clilist, &msg.remote_un, &cli);
                            if(cli) {
                                lmice_trace_info_t* pc = (lmice_trace_info_t*)cli->board.addr;
                                pc->tid = info->tid;
                                pc->pid = info->pid;
                                lmice_info_log("Client[%s] joined at [%s], evt[%s], board[%s].\n",
                                               reg->symbol, msg.remote_un.sun_path,
                                               cli->event.name, cli->board.name);
                            }
                        } else {
                            lmice_error_log("Failed to register client[%s] as [%d]\n", msg.remote_un.sun_path, ret);
                        }
                        break;

                    }
                    case EM_LMICE_SUB_TYPE:
                    {
                        int ret;
                        uint64_t hval;
                        server_t *ser = &g_server;
                        client_t* cli = NULL;
                        pubsub_shm_t *ps = NULL;
                        const lmice_sub_t* pb = (const lmice_sub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;

                        lmice_logging(info, "Subscribe[%s] \n", pb->symbol);

                        hval = eal_hash64_fnv1a(pb->symbol, SYMBOL_LENGTH);
                        ret = lm_shmlist_sub(ser->shmlist, hval, &ps);
                        if(ret == 0) {
                            ret = lm_clientlist_find(ser->clilist, &msg.remote_un, &cli);
                            if(ret == 0) {
                                ret = lm_client_sub(cli, ps, pb->symbol);
                                if(ret == 0) {
                                    lmice_logging(info, "Subscribe[%s] successed [%s]", pb->symbol, msg.remote_un.sun_path);
                                } else {
                                    lmice_logging(info, "Subscribe[%s] full sub error[%s]", pb->symbol, msg.remote_un.sun_path);
                                }
                            } else {
                                lmice_logging(info, "Subscribe[%s] find client failed[%d].\n", pb->symbol, ret);
                            }
                        } else {
                            lmice_logging(info, "Subscribe[%s] find shmlist failed[%d].\n", pb->symbol, ret);
                        }

                        break;

                    }
                    case EM_LMICE_UNSUB_TYPE:
                    {
                        int ret;
                        server_t* ser = &g_server;
                        client_t* cli = NULL;
                        const lmice_unsub_t* pb = (const lmice_unsub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;

                        ret = lm_clientlist_find(ser->clilist, &msg.remote_un, &cli);
                        if(ret == 0) {
                            ret = lm_client_unsub(cli, NULL, pb->symbol);
                            lmice_logging(info, "Unsubscribe[%s] successed [%s]", pb->symbol, msg.remote_un.sun_path);
                        } else {
                            lmice_logging(info, "Unsubscribe[%s] find client failed [%d]", pb->symbol, ret);
                        }

                        break;
                    }
                    case EM_LMICE_PUB_TYPE:
                    {
                        int ret;
                        uint64_t hval;
                        server_t* ser = &g_server;
                        client_t* cli = NULL;
                        pubsub_shm_t *ps = NULL;
                        const lmice_pub_t* pb = (const lmice_pub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;

                        hval = eal_hash64_fnv1a(pb->symbol, SYMBOL_LENGTH);
                        ret = lm_shmlist_pub(ser->shmlist, hval, &ps);
                        if(ret == 0) {
                            ret = lm_clientlist_find(ser->clilist, &msg.remote_un, &cli);
                            if(ret == 0) {
                                ret = lm_client_pub(cli, ps, pb->symbol);
                                if(ret == 0) {
                                    lmice_logging(info,"Publish[%s] successed [%s]", pb->symbol, msg.remote_un.sun_path);
                                } else {
                                    lmice_logging(info,"Publish[%s] full pub failed[%d]\n", pb->symbol, ret );
                                }
                            } else {
                                lmice_logging(info,"Publish[%s] find shmlist failed[%d]", pb->symbol, ret);
                            }
                        } else {
                            lmice_logging(info, "Publish[%s] find shmlist failed[%d]", pb->symbol, ret);
                        }

                        break;
                    }
                    case EM_LMICE_UNPUB_TYPE:
                    {
                        int ret;
                        server_t* ser = &g_server;
                        client_t* cli = NULL;
                        const lmice_unpub_t* pb = (const lmice_unpub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;

                        ret = lm_clientlist_find(ser->clilist, &msg.remote_un, &cli);
                        if(ret == 0) {
                            ret = lm_client_unpub(cli, NULL, pb->symbol);
                            lmice_logging(info, "Unpublish[%s] successed [%s]", pb->symbol, msg.remote_un.sun_path);
                        } else {
                            lmice_logging(info, "Unpublish[%s] find client failed [%d]", pb->symbol, ret);
                        }

                        break;

                    }
                    case EM_LMICE_SEND_DATA:
                    {
                        /** Send data from publisher */
                        server_t *ser = &g_server;
                        client_t *cli = NULL;
                        const lmice_send_data_t* pb = (const lmice_send_data_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;
                        int ret =0;
                        size_t i;

                        uint64_t hval;
                        char name[SYMBOL_LENGTH] ={0};
                        GET_SHMNAME(pb->sub.symbol, SYMBOL_LENGTH, hval, name);

                        lmice_logging(info, "Senddata[%s], client size:%u  sym[%s]", pb->sub.symbol, ser->clilist->count, name);

                        /* Find pub client */
                        ret = lm_clientlist_find(ser->clilist, &msg.remote_un, &cli);
                        if(ret != 0) {
                            lmice_logging(info, "Senddata[%s], cant find client[%s]", pb->sub.symbol, msg.remote_un.sun_path);
                            break;
                        }

                        /* Awake subs */
                        if(ret == 0)
                        {
                            /* Loop sub:clients */
                            size_t j;
                            clientlist_t* cur = ser->clilist;
                            do {
                                for(i=0; i<cur->count; ++i) {
                                    client_t* cli = &cur->cli[i];
                                    for(j=0; j<cli->count; ++j) {
                                        symbol_shm_t *sym = &cli->symshm[j];
                                        pubsub_shm_t* ps = sym->ps;
                                        if(ps->hval == hval && sym->type & SHM_SUB_TYPE) {
                                            /* Add */
                                            lmice_sub_data_t* dt = (lmice_sub_data_t*)((char*)cli->board.addr + CLIENT_SUBPOS);
                                            eal_spin_lock(&dt->lock);
                                            if(dt->count<CLIENT_SPCNT) {
                                                sub_detail_t* sd = dt->sub + dt->count;
                                                memcpy(sd, &pb->sub, sizeof(sub_detail_t));
                                                ++dt->count;
                                            } else if(dt->count > CLIENT_SPCNT) {
                                                dt->count = 0;
                                            }
                                            eal_spin_unlock(&dt->lock);

                                            /* Awake */
                                            eal_event_awake(cli->event.fd);
                                            lmice_logging(info, "Fire senddata event to client[%s]\n", cli->addr.sun_path);

                                            break; /* break-for: cli->count */
                                        }
                                    }/*end-for cli->count */
                                }/*end-for cur->count */
                                cur = cur->next;
                            } while(cur != NULL);
                        }
                        lmice_logging(info, "Senddata[%s], size[%u]", pb->sub.symbol, pb->sub.size);

                    }
                    default:
                        break;
                    }

                    if( LMICE_TRACE_TYPE == (int)(*msg.data) ) {

                    }
                    //                    sendto(events[i].data.fd, msg.data, count, 0, (struct sockaddr*)&(msg.remote_un), addr_len);
                }
            }
        } /*end-for:n */

        now = time(NULL);
        if(now - last >=MAINTAIN_PERIOD) {
            server_t* cur;
            last = now;
            /* for each maintain-second, check client */
            lmice_critical_log("begin maintain client size=%u\n", cur->clilist->count);
            cur = &g_server;
            lm_clientlist_maintain(cur->clilist);
        }
    }/* end-while:1 */

    free (events);

    close(efd);


}


#define QUOTE_FLAG_SUMMARY		4


#pragma pack(push, 1)

struct guava_udp_head
{
    unsigned int	m_sequence;				///<会话编号
    char			m_exchange_id;			///<市场  0 表示中金  1表示上期
    char			m_channel_id;			///<通道编号
    char			m_symbol_type_flag;		///<合约标志
    int				m_symbol_code;			///<合约编号
    char			m_symbol[31];			///<合约
    char			m_update_time[9];		///<最后更新时间(秒)
    int				m_millisecond;			///<最后更新时间(毫秒)
    char			m_quote_flag;			///<行情标志		0 无time sale, 无lev1, 1 有time sale 无lev1, 2 无time sale, 有lev1, 3 有time sale, 有lev1, 4 summary信息
};

struct guava_udp_normal
{
    double			m_last_px;				///<最新价
    int				m_last_share;			///<最新成交量
    double			m_total_value;			///<成交金额
    double			m_total_pos;			///<持仓量
    double			m_bid_px;				///<最新买价
    int				m_bid_share;			///<最新买量
    double			m_ask_px;				///<最新卖价
    int				m_ask_share;			///<最新卖量
};

struct guava_udp_summary
{
    double			m_open;					///<今开盘
    double			m_high;					///<最高价
    double			m_low;					///<最低价
    double			m_today_close;			///<今收盘
    double			m_high_limit;			///<涨停价
    double			m_low_limit;			///<跌停价
    double			m_today_settle;			///<今结算价
    double			m_curr_delta;			///<今虚实度
};

#pragma pack(pop)

int write_msg(const char* msg, int size) {
    char fname[64]={0};
    struct guava_udp_head* ph;
    if(size != sizeof(struct guava_udp_head) + sizeof(struct guava_udp_normal)) {
        lmice_error_log("Message size(%d) wrong, perfer(%lu).", size,
                        sizeof(struct guava_udp_head) + sizeof(struct guava_udp_normal));
        return -1;
    }

    ph = (struct guava_udp_head*)msg;
    return 0;
}
