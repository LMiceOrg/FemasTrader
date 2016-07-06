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

#include <signal.h>
#include <errno.h>

#if defined(__linux__)
#include <sys/epoll.h>
#include <unistd.h>
#endif

#if defined(__MACH__)
#include <kqueue.h>
#endif

#if defined(__MACH__) || defined(__linux__)
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#endif

#if defined(_WIN32)
#include <WinSock2.h>
#endif

#include <string.h>

#define PID_FILE "/var/run/lmiced.pid"
#define SOCK_FILE "/var/run/lmiced.socket"

static volatile int g_quit_flag = 0;

#define MAXEVENTS 64








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


#define GET_SHMNAME(symbol, sym_len, hval, name) do {  \
    hval = eal_hash64_fnv1a(symbol, sym_len);   \
    eal_shm_hash_name(hval, name);  \
    } while(0)



/** Register daemon as service */
static int init_daemon();

static int init_server();

/** event poll version Daemon-Client communication */
static int init_epoll(int sfd);

/** poll version Daemon-Client communication */
static int init_poll(int sfd);

forceinline void proc_msg(uds_msg* msg);

/** Semaphore+shm version Daemon-Client communication */
/* server event thread */
void event_thread(void* p);
/* server symbol event thread */
void symbol_event_thread(void* ptr);
/* server register event thread */
void register_event_thread(void* ptr);

/** process SIGTERM(13) signal */
static void signal_handler(int sig) {
    if(sig == SIGTERM)
        g_quit_flag = 1;
}


int main(int argc, char* argv[]) {
    uint64_t hval;
    int ret;
	eal_thread_t pt[4];
    lm_thread_ctx_t *tctx[4];

    (void)argc;
    (void)argv;

    /* Set mask of process, others & group without exec right */
    umask(011);

    memset(&g_server, 0, sizeof(g_server));

    lmice_info_print("LMice server launching...\n");
    /* Init Daemon */
    ret = init_daemon();
    if(ret != 0) {
        lmice_error_print("Deamon init failed\n");
        return ret;
    }

    g_server.fd = init_server();
    if(g_server.fd < 0) {
        lmice_error_log("Server init failed\n");
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
    ret = eal_shm_create(&g_server.board);
    if(ret != 0) {
        lmice_error_log("Shm board init failed\n");
        return ret;
    }

    /* Create event thread */
    eal_thread_malloc_context(tctx[0]);
    tctx[0]->context = &g_server;
    tctx[0]->handler = event_thread;
    eal_thread_create(&pt[0], tctx[0]);
    //pthread_create(&pt[0], NULL, event_thread, &g_server);

    /* Create symbol thread */
    eal_thread_malloc_context(tctx[1]);
    tctx[1]->context = &g_server;
    tctx[1]->handler = symbol_event_thread;
    eal_thread_create(&pt[1], tctx[1]);
    //pthread_create(&pt[1], NULL, symbol_event_thread, &g_server);

    /* Create register thread */
    eal_thread_malloc_context(tctx[2]);
    tctx[2]->context = &g_server;
    tctx[2]->handler = register_event_thread;
    eal_thread_create(&pt[2], tctx[2]);
    //pthread_create(&pt[2], NULL, register_event_thread, &g_server);

    /* Listen and wait signal to end */
    /*signal(SIGCHLD,SIG_IGN);  ignore child */
    /* signal(SIGTSTP,SIG_IGN);  ignore tty signals */
    signal(SIGTERM,signal_handler); /* catch kill signal 13 */

    lmice_info_log("LMice server running.");

    create_uds_msg((void**)&g_server.pmsg);
    ret = init_uds_server(SOCK_FILE, g_server.pmsg);
    if(ret == 0) {
        /* Init and wait for UDS event */
        init_epoll(g_server.pmsg->sock);
    }

    /* Stop and clean resources */
    lmice_info_log("LMice server stopping...\n");
	eal_thread_join(pt[0], ret);
	eal_thread_join(pt[1], ret);
	eal_thread_join(pt[2], ret);
    //pthread_join(pt[0], NULL);
    //pthread_join(pt[1], NULL);
    //pthread_join(pt[2], NULL);
    ret=0;
    ret |= finit_uds_msg(g_server.pmsg);
    ret |= eal_shm_destroy(&g_server.board);
    ret |= lm_clientlist_delete(g_server.clilist);
    ret |= lm_shmlist_delete(g_server.shmlist);
    ret |= close(g_server.fd);
    ret |= unlink(PID_FILE);
    lmice_info_log("LMice server stopped [%d].\n", ret);

    return ret;
}


int init_daemon() {
    if( daemon(0, 1) != 0) {
        return -2;
    }
    return 0;
}

int init_server() {
    struct flock lock;
    eal_pid_t pid;
    char buf[32] = {0};
    ssize_t sz;

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

int init_poll(int sfd) {
	uds_msg msg = { 0 };
	socklen_t addr_len = sizeof(msg.remote_un);
	struct pollfd fds[1];
	int ret;
	fds[0].fd = sfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	while (1) {
		/* wait 100 milliseconds */
		ret = poll(fds, 1, 100);
		if (g_quit_flag == 1) {
			lmice_info_log("Poll wait quit.");
			break;
		}
		
		if (ret < 0) {
			/* Error occured*/
			lmice_error_log("Poll wait error.");
			break;
		}
		else if (ret == 0) {
			/* Timed out*/
			continue;
		}
		
		msg.size = recvfrom(sfd, msg.data, sizeof msg.data, 0, (struct sockaddr*)&(msg.remote_un), &addr_len);

		if (msg.size > 4) {
			proc_msg(&msg);
		}
		
	}

	return 0;

}

int init_epoll(int sfd) {
    int s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    uds_msg msg = {0};
    socklen_t addr_len=sizeof(msg.remote_un);

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
                for(;;) {
                    msg.size = recvfrom(events[i].data.fd, msg.data, sizeof msg.data, 0, (struct sockaddr*)&(msg.remote_un), &addr_len);
                    if (msg.size == -1)
                    {
                        /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                        if (errno != EAGAIN)
                        {
                            lmice_error_log("Read(socket) error occured.");
                        }
                        break; /*break for-endless-loop */
                    } else {
						/*
						 * lmice_info_log("Read(socket) from %s[%ld] done.", msg.remote_un.sun_path, msg.size);
						 */
						proc_msg(&msg);
                        
                        //sendto(events[i].data.fd, msg.data, count, 0, (struct sockaddr*)&(msg.remote_un), addr_len);
                    }

                }/*end-for:endless-loop */
            }
        } /*end-for:n */

        now = time(NULL);
        if(now - last >=MAINTAIN_PERIOD) {
            server_t* cur;
            last = now;
            /* for each maintain-second, check client */
            /*
             * lmice_critical_log("begin maintain client size=%u\n", cur->clilist->count);
             */
            cur = &g_server;
            lm_clientlist_maintain(cur->clilist);
        }
    }/* end-while:1 */

    free (events);

    close(efd);
    return 0;

}

forceinline void proc_msg(uds_msg* msg) {
	switch ((int)(*msg->data)) {
	case EM_LMICE_TRACE_TYPE:
	{
		const lmice_trace_info_t* info = (const lmice_trace_info_t*)msg->data;
		const char* data = (const char*)(msg->data + sizeof(lmice_trace_info_t));
		lmice_logging(info, data);
		break;
	}
	case EMZ_LMICE_TRACEZ_BSON_TYPE:
	{
		const lmice_trace_bson_info_t* info = (const lmice_trace_bson_info_t*)msg->data;
		const char* data = (const char*)(msg->data + sizeof(lmice_trace_bson_info_t));
		unsigned int length = *(const unsigned int*)data;
		//bson data length is default little endia
		//memcpy(&length, data, sizeof(length));
		lmice_logging_bson(info, data, length);
		break;
	}
	case EM_LMICE_REGCLIENT_TYPE:
	{
		const lmice_register_t* reg = (const lmice_register_t*)msg->data;
		const lmice_trace_info_t* info = &reg->info;
		server_t* ser = &g_server;
		client_t* cli = NULL;
		/* Register client */
		int ret = 0;
		/* Find or create client */
		ret = lm_clientlist_register(ser->clilist, info->tid, info->pid, reg->symbol, &msg->remote_un, &cli);
		if (ret == 0) {
			lmice_info_log("Client[%s] joined at [%s], evt[%s], board[%s].\n",
				reg->symbol, msg->remote_un.sun_path,
				cli->event.name, cli->board.name);

		}
		else {
			lmice_error_log("Failed to register client[%s] as [%d]\n", msg->remote_un.sun_path, ret);
		}
		break;

	}
	case EM_LMICE_UNREGCLIENT_TYPE:
	{
		int ret;
		server_t* ser = &g_server;
		client_t* cli = NULL;
		ret = lm_clientlist_unregister(ser->clilist, &msg->remote_un, &cli);
		if (ret == 0 && cli) {
			lmice_info_log("Client[%s] leaved at [%s] pid[%d], board[%s].\n",
				cli->name, cli->addr.sun_path,
				cli->pid, cli->board.name);
		}
		else {
			lmice_error_log("Failed to unregister client[%s] as [%d]\n", msg->remote_un.sun_path, ret);
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
		const lmice_sub_t* pb = (const lmice_sub_t*)msg->data;
		const lmice_trace_info_t* info = &pb->info;

		hval = eal_hash64_fnv1a(pb->symbol, SYMBOL_LENGTH);
		ret = lm_shmlist_sub(ser->shmlist, hval, &ps);
		if (ret == 0) {
			ret = lm_clientlist_find(ser->clilist, &msg->remote_un, &cli);
			if (ret == 0) {
				ret = lm_client_sub(cli, ps, pb->symbol);
				if (ret == 0) {
					lmice_logging(info, "Subscribe[%s] successed [%s]", pb->symbol, msg->remote_un.sun_path);
				}
				else {
					lmice_logging(info, "Subscribe[%s] full sub error[%s]", pb->symbol, msg->remote_un.sun_path);
				}
			}
			else {
				lmice_logging(info, "Subscribe[%s] find client failed[%d].\n", pb->symbol, ret);
			}
		}
		else {
			lmice_logging(info, "Subscribe[%s] find shmlist failed[%d].\n", pb->symbol, ret);
		}

		break;

	}
	case EM_LMICE_UNSUB_TYPE:
	{
		int ret;
		server_t* ser = &g_server;
		client_t* cli = NULL;
		const lmice_unsub_t* pb = (const lmice_unsub_t*)msg->data;
		const lmice_trace_info_t* info = &pb->info;

		ret = lm_clientlist_find(ser->clilist, &msg->remote_un, &cli);
		if (ret == 0) {
			ret = lm_client_unsub(cli, NULL, pb->symbol);
			lmice_logging(info, "Unsubscribe[%s] successed [%s]", pb->symbol, msg->remote_un.sun_path);
		}
		else {
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
		const lmice_pub_t* pb = (const lmice_pub_t*)msg->data;
		const lmice_trace_info_t* info = &pb->info;

		hval = eal_hash64_fnv1a(pb->symbol, SYMBOL_LENGTH);
		ret = lm_shmlist_pub(ser->shmlist, hval, &ps);
		if (ret == 0) {
			ret = lm_clientlist_find(ser->clilist, &msg->remote_un, &cli);
			if (ret == 0) {
				ret = lm_client_pub(cli, ps, pb->symbol);
				if (ret == 0) {
					lmice_logging(info, "Publish[%s] successed [%s]", pb->symbol, msg->remote_un.sun_path);
				}
				else {
					lmice_logging(info, "Publish[%s] full pub failed[%d]\n", pb->symbol, ret);
				}
			}
			else {
				lmice_logging(info, "Publish[%s] find shmlist failed[%d]", pb->symbol, ret);
			}
		}
		else {
			lmice_logging(info, "Publish[%s] find shmlist failed[%d]", pb->symbol, ret);
		}

		break;
	}
	case EM_LMICE_UNPUB_TYPE:
	{
		int ret;
		server_t* ser = &g_server;
		client_t* cli = NULL;
		const lmice_unpub_t* pb = (const lmice_unpub_t*)msg->data;
		const lmice_trace_info_t* info = &pb->info;

		ret = lm_clientlist_find(ser->clilist, &msg->remote_un, &cli);
		if (ret == 0) {
			ret = lm_client_unpub(cli, NULL, pb->symbol);
			lmice_logging(info, "Unpublish[%s] successed [%s]", pb->symbol, msg->remote_un.sun_path);
		}
		else {
			lmice_logging(info, "Unpublish[%s] find client failed [%d]", pb->symbol, ret);
		}

		break;

	}
	case EM_LMICE_SEND_DATA:
	{
		/** Send data from publisher */
		server_t *ser = &g_server;
		client_t *cli = NULL;
		const lmice_send_data_t* pb = (const lmice_send_data_t*)msg->data;
		const lmice_trace_info_t* info = &pb->info;
		int ret = 0;
		size_t i;
		pubsub_shm_t *pps = NULL;
		int64_t begintm;
		get_system_time(&begintm);

		/*
		lmice_logging(info, "Senddata[%s], client size:%u  sym[%s]", pb->sub.symbol, ser->clilist->count, name);
		*/
		/* Find pub client */
		ret = lm_shmlist_find(ser->shmlist, pb->sub.hval, &pps);
		if (!pps) {
			lmice_logging(info, "Senddata[%lu], cant find client[%s]", pb->sub.hval, msg->remote_un.sun_path);
			break;
		}

		/* Loop sub:clients */

		clientlist_t* cur = ser->clilist;
		do {
			size_t j;
			for (i = 0; i<cur->count; ++i) {
				client_t* cli = &cur->cli[i];
				for (j = 0; j<cli->count; ++j) {
					symbol_shm_t *sym = &cli->symshm[j];
					pubsub_shm_t* ps = sym->ps;
					if (ps->hval == pb->sub.hval && sym->type & SHM_SUB_TYPE) {
						/* Add */
						lmice_sub_data_t* dt = (lmice_sub_data_t*)((char*)cli->board.addr + CLIENT_SUBPOS);
						eal_spin_lock(&dt->lock);
						if (dt->count <= CLIENT_SPCNT) {
							sub_detail_t* sd = dt->sub + dt->count;
							memcpy(sd, &pb->sub, sizeof(sub_detail_t));
							++dt->count;
						}
						else if (dt->count > CLIENT_SPCNT) {
							dt->count = 0;
						}
						eal_spin_unlock(&dt->lock);

						/* Awake */
						eal_event_awake(cli->event.fd);
						{
							int64_t now;
							get_system_time(&now);
							lmice_logging(info,
								"senddata from[%ld] to[%ld]\n",
								begintm,
								now);
						}

						break; /* break-for: cli->count */
					}
				}/*end-for cli->count */
			}/*end-for cur->count */
			cur = cur->next;
		} while (cur != NULL);

		break;
	}
	default:
		break;
	}
}

void register_event_thread(void* ptr) {
    int ret;
    int cnt;
    int i;
    server_t *ser = (server_t*)ptr;
    evtfd_t efd = (evtfd_t)((char*)ser->board.addr+ SERVER_REGEVT);
    lmice_register_data_t* pd = (lmice_register_data_t*)((char*)ser->board.addr+SERVER_REGPOS);
    lmice_register_detail_t pdlist[REGLIST_LENGTH];

    ret = eal_eventp_init(efd);
    if(ret != 0)
        return;

    lmice_info_log("Begin register event thread\n");
    for(;;) {
        /* Wait and timeout set to 100 ms */
        ret = eal_event_wait_timed(efd, 100);

        /* Check quit flag */
        if(g_quit_flag == 1)
            break;

        /* Time out */
        if(ret == -1 && errno == ETIMEDOUT)
            continue;

        /* Check register message */
        eal_spin_lock(&pd->lock);
        cnt = pd->count;
        if(cnt >0 && cnt <= REGLIST_LENGTH) {
            memcpy(pdlist, pd->reg, cnt*sizeof(lmice_register_detail_t));
            pd->count = 0;
        } else if(cnt > REGLIST_LENGTH) { /* error we have */
            cnt = 0;
            pd->count = 0;
        }
        eal_spin_unlock(&pd->lock);
        /* Process message */
        for(i=0; i<cnt; ++i) {
            client_t* cli = NULL;
            lmice_register_detail_t* detail = pdlist+i;
            switch(detail->type) {
            case EM_LMICE_REGCLIENT_TYPE:
                /* Find or create client */
                ret = lm_clientlist_register(ser->clilist, detail->tid, detail->pid, detail->symbol, &detail->un, &cli);
                if(ret == 0 && cli) {
                    lmice_info_log("Client[%s] joined at [%s], pid[%d], board[%s].\n",
                                   cli->name, cli->addr.sun_path,
                                   cli->pid, cli->board.name);

                } else {
                    lmice_error_log("Failed to register client[%s] as [%d]\n", detail->un.sun_path, ret);
                }
                break;
            case EM_LMICE_UNREGCLIENT_TYPE:
                ret = lm_clientlist_unregister(ser->clilist, &detail->un, &cli);
                if(ret == 0 && cli) {
                    lmice_info_log("Client[%s] leaved at [%s] pid[%d], board[%s].\n",
                                   cli->name, cli->addr.sun_path,
                                   cli->pid, cli->board.name);
                } else {
                    lmice_error_log("Failed to unregister client[%s] as [%d]\n", detail->un.sun_path, ret);
                }
                break;
            default:
                break;
            } /*end-switch: type*/

        }/*end-for:i*/

    }/* end-for:endless-loop */

    eal_eventp_close(efd);
    return;
}

void symbol_event_thread(void* ptr) {
    int ret;
    server_t *ser = (server_t*)ptr;
    evtfd_t efd = (evtfd_t)((char*)ser->board.addr+SERVER_EVTPOS + SERVER_SYMEVT);
    lmice_symbol_data_t* sym = (lmice_symbol_data_t*)((char*)ser->board.addr+SERVER_SYMPOS);
    lmice_symbol_detail_t symlist[SYMLIST_LENGTH];

    int isym;

    ret = eal_eventp_init(efd);
    if(ret != 0)
        return;

    lmice_info_log("Begin symbol event thread\n");
    for(;;) {
        /* Wait and timeout set to 100 ms */
        ret = eal_event_wait_timed(efd, 100);

        /* Check quit flag */
        if(g_quit_flag == 1)
            break;

        /* Time out */
        if(ret == -1 && errno == ETIMEDOUT)
            continue;

        /* Check publish message */
        eal_spin_lock(&sym->lock);
        ret = sym->count;
        if(ret >0 && ret <= SYMLIST_LENGTH) {
            memcpy(symlist, sym->sym, ret*sizeof(lmice_symbol_detail_t));
            sym->count = 0;
        } else if(ret > SYMLIST_LENGTH) { /* error we have */
            ret = 0;
            sym->count = 0;
        }
        eal_spin_unlock(&sym->lock);

        for(isym = 0; isym<ret; ++isym) {
            client_t* cli = NULL;
            pubsub_shm_t * ps = NULL;
            lmice_symbol_detail_t* pd = &symlist[isym];

            /* Find the client by pid */
            lm_clientlist_find_pid(ser->clilist, pd->pid, &cli);
            if(!cli) {
                continue;
            }

            /* Process symbol operator:sub/unsub, pub/unpub */
            switch(pd->type) {
            case EM_LMICE_SUB_TYPE:
                ret = lm_shmlist_sub(ser->shmlist, pd->hval, &ps);
                if(ret == 0) {
                    ret = lm_client_sub(cli, ps, pd->symbol);
                    if(ret == 0) {
                        lmice_info_log("Subscribe[%s] successed [%s][%d]", pd->symbol, cli->name, pd->pid);
                    }
                }
                break;
            case EM_LMICE_UNSUB_TYPE:
                ret = lm_client_unsub(cli, NULL, pd->symbol);
                if(ret == 0) {
                    lmice_info_log("Unsubscribe[%s] successed [%s][%d]", pd->symbol, cli->name, pd->pid);
                }
                break;
            case EM_LMICE_PUB_TYPE:
                ret = lm_shmlist_pub(ser->shmlist, pd->hval, &ps);
                if(ret == 0) {
                    ret = lm_client_pub(cli, ps, pd->symbol);
                    if(ret == 0) {
                        lmice_info_log("Publish[%s] successed [%s][%d]", pd->symbol, cli->name, pd->pid);
                    }
                }
                break;
            case EM_LMICE_UNPUB_TYPE:
                ret = lm_client_unpub(cli, NULL, pd->symbol);
                if(ret == 0) {
                    lmice_info_log("Unpublish[%s] successed [%s][%d]", pd->symbol, cli->name, pd->pid);
                }
                break;
            default:
                break;
            } /*end-switch:type */
        }/*end-for: isym list */

    } /*end-for:endless-loop */

    eal_eventp_close(efd);
    return;
}

void event_thread(void* ptr) {
    int ret;
    server_t *ser = (server_t*)ptr;
    evtfd_t efd = (evtfd_t)((char*)ser->board.addr+SERVER_EVTPOS);
    lmice_pub_data_t* pub = (lmice_pub_data_t*)((char*)ser->board.addr+SERVER_PUBPOS);
    pub_detail_t publist[PUBLIST_LENGTH];
    size_t i;
    size_t j;
    size_t ipub;

    ret = eal_eventp_init(efd);
    if(ret != 0)
        return;

    lmice_info_log("Begin event thread\n");

    memset(publist, 0, sizeof(pub_detail_t)*64);
    for(;;) {
        /* Wait and timeout set to 100 ms */
        ret = eal_event_wait_timed(efd, 100);

        /* Check quit flag */
        if(g_quit_flag == 1)
            break;
        if(ret == -1 && errno == ETIMEDOUT) {
            /* time out */
        } else {

            /* Check publish message */
            eal_spin_lock(&pub->lock);
            ret = pub->count;
            if(ret >0 && ret <= PUBLIST_LENGTH) {
                memcpy(publist, pub->pub, ret*sizeof(pub_detail_t));
                pub->count = 0;
            } else if(ret > PUBLIST_LENGTH) {
                ret = PUBLIST_LENGTH;
                memcpy(publist, pub->pub, ret*sizeof(pub_detail_t));
                pub->count -= PUBLIST_LENGTH;
            }
            eal_spin_unlock(&pub->lock);

            /*lmice_info_log("event fired, size=%d, hval=%lu\n", ret, publist[0].hval);
             */
            ipub = 0;
            for(ipub =0; ipub < ret; ++ipub) {
            /* Awake subscribers */
            for(i=0; i< ser->clilist->count; ++i) {
                client_t* cli = &ser->clilist->cli[i];
                for(j=0; j<cli->count; ++j) {
                    symbol_shm_t* sym = &cli->symshm[j];
                    pubsub_shm_t* shm = sym->ps;
                    if(shm->hval == publist[ipub].hval && sym->type & SHM_SUB_TYPE) {

                        lmice_sub_data_t* dt = (lmice_sub_data_t*)((char*)cli->board.addr + CLIENT_SUBPOS);
                        eal_spin_lock(&dt->lock);
                        if(dt->count<= CLIENT_SPCNT) {
                            sub_detail_t* sd = dt->sub + dt->count;
                            memcpy(sd, publist+ipub, sizeof(pub_detail_t));
                            ++dt->count;
                        } else if(dt->count > CLIENT_SPCNT) {
                            dt->count = 0;
                        }
                        eal_spin_unlock(&dt->lock);

                        eal_event_awake(cli->event.fd);
                        /*lmice_info_log("event sub fired\n");*/
                        break;
                    }
                }
            }
            }


        }

    }/* for: ;; */

    eal_eventp_close(efd);
    return;
}
