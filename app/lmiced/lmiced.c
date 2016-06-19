#include "udss.h"

#include "lmice_trace.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_shm.h"
#include "lmice_eal_hash.h"
#include "lmice_eal_event.h"

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
struct symbolnode {
    struct sockaddr_un addr;
    socklen_t addr_len;
    char symbol[32];
    lmice_event_t event;
    struct symbolnode* next;

};

struct appclient {
    struct sockaddr_un addr;
    char symbol [32];

};

struct symbolnode sublist;
struct symbolnode publist;

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
                        unsigned int length = 0;
                        //bson data length is default little endia
                        memcpy(&length, data, sizeof(length));
                        lmice_logging_bson(info, data, length);
                        break;
                    }
                    case EM_LMICE_SUB_TYPE:
                    {
                        const lmice_sub_t* psub = (const lmice_sub_t*)msg.data;
                        const lmice_trace_info_t* info = &psub->info;
                        lmice_logging(info, "Subscribe symbol[%s]", psub->symbol);
                        struct symbolnode* parent = &sublist;
                        struct symbolnode* node = sublist.next;

                        while(node!=NULL) {
                            if(memcmp(&(node->addr), &(msg.remote_un),node->addr_len) == 0
                                    && strcmp(node->symbol, psub->symbol) == 0) {
                                lmice_info_log("Symbol[%s] already subscribed by client %s"
                                               , node->symbol
                                               , msg.remote_un.sun_path);
                                break;
                            }
                            parent = node;
                            node = node->next;
                        }
                        if(node == NULL) {
                            uint64_t hval=0;
                            node = (struct symbolnode*)malloc(sizeof(struct symbolnode));
                            memset(node, 0, sizeof(struct  symbolnode));
                            memcpy(&(node->addr), &(msg.remote_un), addr_len);
                            node->addr_len = addr_len;
                            node->next = NULL;
                            memcpy(node->symbol, psub->symbol, sizeof(node->symbol));
                            hval = eal_hash64_fnv1a((const char*)&(msg.remote_un),node->addr_len);
                            eal_event_hash_name(hval, node->event->name);
                            ret = eal_event_create(&node->event);
                            if(ret != 0) {
                                lmice_critical_log("EAL create event[%s] failed as[%d]\n", (const char*)&(msg.remote_un), ret);
                            }
                            parent->next = node;
                        }

                        break;

                    }
                    case EM_LMICE_UNSUB_TYPE:
                    {
                        const lmice_unsub_t* psub = (const lmice_unsub_t*)msg.data;
                        const lmice_trace_info_t* info = &psub->info;
                        lmice_logging(info, "Unsubscribe symbol[%s]", psub->symbol);
                        struct symbolnode* parent = &sublist;
                        struct symbolnode* node = sublist.next;
                        do {
                            if(memcmp(&(node->addr), &(msg.remote_un),node->addr_len) == 0
                                    && strcmp(node->symbol, psub->symbol) == 0) {
                                parent->next = node->next;
                                free(node);
                                break;
                            }
                            parent = node;
                            node = node->next;
                        } while(node != NULL);

                        break;
                    }
                    case EM_LMICE_PUB_TYPE:
                    {
                        const lmice_pub_t* ppub = (const lmice_pub_t*)msg.data;
                        const lmice_trace_info_t* info = &ppub->info;
                        lmice_logging(info, "Publish symbol[%s]", ppub->symbol);
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
