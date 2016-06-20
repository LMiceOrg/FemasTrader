#include "udss.h"

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

enum symbol_client_e{
    SYMBOL_LENGTH=32,
    CLIENT_BOARD = 4*1024,  /* 4KB */
    CLIENT_COUNT = 64,
    SYMBOL_SHMSIZE = 8*1024*1024, /* 8MB */
    CLIENT_SUBPOS = 1024,
    CLIENT_SUBCNT = 16,
};

/** sub list */
struct client_s {
    int64_t lock;
    lmice_event_t event;
    lmice_shm_t board;
    struct sockaddr_un addr;
    socklen_t addr_len;
    unsigned short count;
    lmice_shm_t resshm[16];

};
typedef struct client_s client_t;

struct symbolnode {
    int64_t lock;
    struct sockaddr_un addr;
    unsigned short count;
    client_t *clients[CLIENT_COUNT];
    char symbol[SYMBOL_LENGTH];
    lmice_event_t event;
    struct symbolnode* next;

};
typedef struct symbolnode server_t;


/** Global publish/subscribe list */
server_t sublist;
server_t publist;

forceinline int find_client(server_t* cur, struct sockaddr_un* addr, client_t** client) {
    size_t i;
    do {
        for(i=0; i<cur->count; ++i) {
            client_t* cli = cur->clients[i];
            if( memcmp(&cli->addr, addr, cli->addr_len) == 0) {
                *client = cli;
                return 0;
            }
        }
        cur = cur->next;
    } while(cur != NULL);
    *client = NULL;
    return -1;
}

forceinline int append_client(server_t* cur, client_t** client) {
    do {
        if(cur->count < CLIENT_COUNT) {
            client_t* cli = (client_t*)malloc(sizeof(client_t));
            cur->clients[cur->count] = cli;
            cur->count ++;
            *client = cli;
            return 0;
        } else {
            server_t* ser = (server_t*)malloc(sizeof(server_t));
            memset(ser, 0, sizeof(server_t));
            cur->next = ser;
        }
        cur = cur->next;
    } while(cur != NULL);
    *client = NULL;
    return -1;
}

forceinline int init_board(client_t* client, struct sockaddr_un *addr, socklen_t addr_len) {
    int ret;
    uint64_t hval;
    hval = eal_hash64_fnv1a(addr, addr_len);
    eal_shm_hash_name(hval, client->board.name);
    client->board.size = CLIENT_BOARD;
    ret = eal_shm_create(&client->board);
    if(ret != 0) {
        lmice_error_log("EAL create board[%s] failed as[%d]\n", addr->sun_path, ret);
    }
    return ret;
}

forceinline int init_client(client_t* client, struct sockaddr_un *addr, socklen_t addr_len) {
    int ret;
    uint64_t hval;
    memcpy(&client->addr, addr, addr_len);
    client->addr_len = addr_len;
    client->count = 0;
    client->lock = 0;
    hval = eal_hash64_fnv1a(addr, addr_len);
    eal_event_hash_name(hval, client->event.name);
    ret = eal_event_create(&client->event);
    if(ret != 0) {
        lmice_error_log("EAL create event[%s] failed as[%d]\n", addr->sun_path, ret);
    }
    ret |= init_board(client, addr, addr_len);
    return ret;
}

#define GET_SHMNAME(symbol, sym_len, hval, name) do {  \
    hval = eal_hash64_fnv1a(symbol, sym_len);   \
    eal_shm_hash_name(hval, name);  \
    } while(0)

forceinline int append_symbol(client_t* client, const char* symbol, size_t sym_len) {
    int ret = 1;
    size_t i;
    uint64_t hval = 0;
    char name[32] = {0};
    GET_SHMNAME(symbol, sym_len, hval, name);

    for(i=0; i< client->count; ++i) {
        lmice_shm_t* shm = &client->resshm[i];
        if(memcmp(shm->name, name, 32) == 0) {
            /* Already appended! */
            lmice_critical_log("Client[%s] already append the symbol[%s]\n", client->addr.sun_path, symbol);
            ret = 0;
            break;
        }
    }
    if(ret == 1) {
        /* Find nothing, so create new symbol */
        if(client->count < SYMBOL_LENGTH) {
            lmice_shm_t* shm = &client->resshm[client->count];
            ++client->count;
            eal_shm_zero(&shm);
            eal_shm_hash_name(hval, shm->name);
            shm->size = SYMBOL_SHMSIZE;
            ret = eal_shm_create_or_open(shm);
            if(ret != 0) {
                lmice_error_log("Create shm[%s] failed as [%d].", name, ret);
            }
        } else {
            lmice_error_log("Client[%s] symbol list is full, can't append symbol[%s]\n", client->addr.sun_path, symbol);
        }
    }
    return ret;
}

forceinline int remove_symbol(client_t *client, const char* symbol, size_t sym_len) {
    int ret;
    size_t i;
    uint64_t hval = 0;
    char name[32] = {0};
    GET_SHMNAME(symbol, sym_len, hval, name);
    for(i=0; i<client->count; ++i) {
        lmice_shm_t* shm = &client->resshm[client->count];
        if(memcmp(shm->name, name, 32) == 0) {
            ret = eal_shm_destroy(shm);
            if(ret != 0) {
                lmice_error_log("remove shm[%s] failed as [%d].", shm->name, ret);
            }
            memmove(shm, shm+1, (client->count-i-1)*sizeof(lmice_shm_t) );
            --client->count;
            ret = 0;
            break;
        }
    }

    return ret;
}

forceinline int removeall_symbol(client_t* client) {
    int ret = 0;
    size_t i;
    for(i=0; i<client->count; ++i) {
        lmice_shm_t* shm = &client->resshm[client->count];
        ret = eal_shm_destroy(shm);
        if(ret != 0) {
            lmice_error_log("remove shm[%s] failed as [%d].", shm->name, ret);
        }
    }
    client->count = 0;
    return ret;
}

#define CLI_APPENDSYMBOL(ser,cli, msg, addr_len, sym, sym_len) do { \
    int ret = 0;    \
    /* Find or create client */ \
    ret = find_client(ser, &msg.remote_un, &cli);   \
    if(ret != 0) {  \
        /* Don't found, so append new client */ \
        ret = append_client(ser, &cli); \
        if(ret != 0) {  \
            lmice_error_log("Create client failed[%d]\n", ret);  \
        } else {                                                \
            init_client(cli, &msg.remote_un, addr_len);          \
        }                                                       \
    }                                                           \
    /* Append symbol */ \
    append_symbol(cli, sym, sym_len); \
    } while(0)

#define CLI_REMOVESYMBOL(ser, cli, msg, sym, sym_len) do{  \
    int ret =0; \
    /* Find client */   \
    ret = find_client(ser, &msg.remote_un, &cli);   \
    if(ret == 0) {  \
        /* Remove symbol */ \
        remove_symbol(cli, sym, sym_len); \
    }   \
    } while(0)

static int init_daemon();
int init_epoll(int sfd);

/* write hugepage */
int write_msg(const char* msg, int size);

void signal_handler(int sig) {
    if(sig == SIGTERM)
        g_quit_flag = 1;
}

int main(int argc, char* argv[]) {
    uds_msg* pmsg;
    struct timespec ts;
    uint64_t hval;
    lmice_shm_t board;
    int rt;
    int fd;
    lmice_info_print("LMice server launching...\n");
    /* Init Daemon */
    fd = init_daemon();
    if(fd < 0) {
        lmice_error_log("Daemon init failed\n");
        return fd;
    }

    /* Create Hugepage SharedMemory */
    eal_shm_zero(&board);
    hval = eal_hash64_fnv1a(BOARD_NAME, sizeof(BOARD_NAME)-1);
    eal_shm_hash_name(hval, board.name);
    board.size = 4096; /* 4K bytes*/
    rt = eal_shm_create(&board);
    if(rt != 0) {
        lmice_error_log("Shm board init failed\n");
        return rt;
    }

    /* Listen and wait to end */
    /*signal(SIGCHLD,SIG_IGN);  ignore child */
    /* signal(SIGTSTP,SIG_IGN);  ignore tty signals */
    signal(SIGTERM,signal_handler); /* catch kill signal */

    ts.tv_sec = 0;
    ts.tv_nsec = 10000000L;

    lmice_info_log("LMice server running.");

    create_uds_msg(&pmsg);
    init_uds_server(SOCK_FILE, pmsg);

    /* init pub/sub list */
    memset(&sublist, 0, sizeof(sublist));
    memset(&publist, 0, sizeof(publist));

    init_epoll(pmsg->sock);

    /* Stop and clean resources */
    finit_uds_msg(pmsg);
    lmice_info_log("LMice server stopped.");
    rt=0;
    rt |= eal_shm_destroy(&board);
    rt |= close(fd);
    rt |= unlink(PID_FILE);
    return rt;
}


int init_daemon() {
    struct flock lock;
    eal_pid_t pid;
    char buf[32] = {0};

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
    write(fd, buf, strlen(buf));

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
    while (1)
    {
        int n, i;

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
            continue;
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
                int ret;
                ssize_t count;
                uds_msg msg = {0};
                socklen_t addr_len=sizeof(msg.remote_un);
                msg.size = recvfrom(events[i].data.fd, msg.data, sizeof msg.data, 0, (struct sockaddr*)&(msg.remote_un), &addr_len);
                if (count == -1)
                {
                    /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                    if (errno != EAGAIN)
                    {
                        lmice_error_log("Read(socket) error occured.");
                    }
                } else {
                    lmice_info_log("Read(socket) from %s[%d] done.", msg.remote_un.sun_path, addr_len);
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
                        const lmice_trace_bson_info_t* info = (const lmice_trace_info_t*)msg.data;
                        const char* data = (const char*)(msg.data + sizeof(lmice_trace_bson_info_t));
                        unsigned int length = *(const unsigned int*)data;
                        //bson data length is default little endia
                        //memcpy(&length, data, sizeof(length));
                        lmice_logging_bson(info, data, length);
                        break;
                    }
                    case EM_LMICE_SUB_TYPE:
                    {
                        server_t *ser = &sublist;
                        client_t* cli = NULL;
                        const lmice_sub_t* pb = (const lmice_sub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;
                        lmice_logging(info, "Subscribe symbol[%s]", pb->symbol);

                        CLI_APPENDSYMBOL(ser,cli, msg, addr_len, pb->symbol, strlen(pb->symbol));


                        break;

                    }
                    case EM_LMICE_UNSUB_TYPE:
                    {
                        server_t* ser = &sublist;
                        client_t* cli = NULL;
                        const lmice_unsub_t* pb = (const lmice_unsub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;
                        lmice_logging(info, "Unsubscribe symbol[%s]", pb->symbol);

                        CLI_REMOVESYMBOL(ser, cli, msg, pb->symbol, strlen(pb->symbol));

                        break;
                    }
                    case EM_LMICE_PUB_TYPE:
                    {
                        server_t* ser = &publist;
                        client_t* cli = NULL;
                        const lmice_pub_t* pb = (const lmice_pub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;
                        lmice_logging(info, "Publish symbol[%s]", pb->symbol);

                        CLI_APPENDSYMBOL(ser,cli, msg, addr_len, pb->symbol, strlen(pb->symbol));

                        break;
                    }
                    case EM_LMICE_UNPUB_TYPE:
                    {
                        server_t* ser = &publist;
                        client_t* cli = NULL;
                        const lmice_pub_t* pb = (const lmice_pub_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;
                        lmice_logging(info, "Publish symbol[%s]", pb->symbol);

                        CLI_REMOVESYMBOL(ser, cli, msg, pb->symbol, strlen(pb->symbol));

                        break;

                    }
                    case EM_LMICE_SEND_DATA:
                    {
                        /** Send data from publisher */
                        server_t *ser = &publist;
                        client_t *cli = NULL;
                        const lmice_send_data_t* pb = (const lmice_send_data_t*)msg.data;
                        const lmice_trace_info_t* info = &pb->info;
                        int ret =0;
                        size_t i;
                        lmice_shm_t *shm;
                        uint64_t hval;
                        char name[32] ={0};
                        GET_SHMNAME(pb->symbol, strlen(pb->symbol), hval, name);

                        /* Find pub client */
                        ret = find_client(ser, &msg.remote_un, &cli);
                        if(ret != 0) {
                            break;
                        }

                        /* Update pub data */
                        ret = 1;
                        for(i=0; i<cli->count; ++i) {
                            shm = &cli->resshm[i];
                            if(memcmp(shm->name, name, 32) == 0) {
                                memcpy(shm->addr, pb->data, pb->size);
                                ret = 0;
                                break;
                            }
                        }
                        /* Awake subs */
                        if(ret == 0)
                        {
                            /* Loop sub:clients */
                            server_t *cur = &sublist;
                            size_t j;
                            do {
                                for(i=0; i<cur->count; ++i) {
                                    client_t* cli = cur->clients[i];
                                    if(cli == NULL)
                                        continue;
                                    for(j=0; j<cli->count; ++j) {
                                        shm = &cli->resshm[j];
                                        if(memcmp(shm->name, name, 32) == 0) {
                                            /* Add */
                                            sub_data_t* dt = (sub_data_t*)((char*)cli->board.addr + CLIENT_SUBPOS);
                                            eal_spin_lock(&dt->lock);
                                            if(dt->count<CLIENT_SUBCNT) {
                                                char* sym = dt->symbol+SYMBOL_LENGTH*dt->count;
                                                memcpy(sym, pb->symbol, SYMBOL_LENGTH);
                                                ++dt->count;
                                            }
                                            eal_spin_unlock(&dt->lock);

                                            /* Awake */
                                            eal_event_awake(cli->event.fd);

                                            break; /* break-for: cli->count */
                                        }
                                    }
                                }
                                cur = cur->next;
                            } while(cur != NULL);
                        }
                        lmice_logging(info, "Send data symbol[%s], size[%u]", pb->symbol, pb->size);



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
        lmice_error_log("Message size(%d) wrong, perfer(%u).", size,
                        sizeof(struct guava_udp_head) + sizeof(struct guava_udp_normal));
        return -1;
    }

    ph = (struct guava_udp_head*)msg;
    return 0;
}
