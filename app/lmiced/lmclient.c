#include "lmclient.h"

#include "lmice_trace.h"
#include "lmice_eal_spinlock.h"
#include "lmice_eal_hash.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define CLIENT_RECOVER_RESOURCE(cli, ret) do{    \
    lmice_shm_t *shm = &cli->board; \
    lmice_event_t *event = &cli->event; \
    ret = eal_shm_destroy(shm); \
    ret |= eal_event_destroy(event);    \
} while(0)

#define CLIENT_RECOVER_PUB_RIGHT(cli) do{\
    size_t j;   \
    for(j=0; j< cli->count; ++j) {  \
        symbol_shm_t *ss = &cli->symshm[j]; \
        pubsub_shm_t *ps = ss->ps;  \
        if(ss->type & SHM_PUB_TYPE) {   \
            ps->type &= (~SHM_PUB_TYPE);   \
        }   \
    }   \
}while(0)

#define CLIENT_TERM_PROCESS(cli, ret) do{    \
    ret = kill(cli->pid, 0);    \
    if(ret == 0) {  \
        kill(cli->pid, SIGINT);    \
        usleep(100000); \
        ret = kill(cli->pid, 0);    \
        if(ret == 0){   \
            ret = kill(cli->pid, SIGSTOP);    \
            usleep(10000);   \
        }   \
    }   \
} while(0)

clientlist_t *lm_clientlist_create()
{
    clientlist_t * cl = (clientlist_t*)malloc(sizeof(clientlist_t));
    memset(cl, 0, sizeof(clientlist_t));
    return cl;
}


int lm_clientlist_delete(clientlist_t *cl)
{
    int ret = 0;
    clientlist_t *next = cl;

    do {
        clientlist_t * cur = next;
        size_t i=0;
        for(i=0; i<cur->count; ++i) {
            client_t* cli = &cur->cli[i];
            if(cli->active == CLIENT_DEAD)
                continue;

            /* Terminate process */
            CLIENT_TERM_PROCESS(cli, ret);

            /* Recover client resource */
            CLIENT_RECOVER_RESOURCE(cli, ret);

            /* Recover pub right */
            CLIENT_RECOVER_PUB_RIGHT(cli);

            if(ret != 0) {
                lmice_error_log("Destroy client[%s] AKA[%s] failed as[%d].\n", cli->addr.sun_path, cli->name, ret);
            }
        }/* end-for: cur->count */

        next = cur->next;
        free(cur);
    } while(next != NULL);

    return ret;
}


int lm_clientlist_register(clientlist_t *cl, pthread_t tid, pid_t pid, const char *name, struct sockaddr_un *addr, client_t **ppc)
{
    int ret = 0;
    client_t* cli = NULL;
    ret = lm_clientlist_find_or_create(cl, addr, &cli);
    if(ret == 0 && cli != NULL) {
        cli->active = CLIENT_ALIVE;
        cli->tid = tid;
        cli->pid = pid;
        memcpy(cli->name, name, SYMBOL_LENGTH);
        *ppc = cli;
    }
    return ret;
}


int lm_clientlist_unregister(clientlist_t *cl, struct sockaddr_un *addr, client_t** ppc)
{
    int ret =0;
    client_t* cli = NULL;
    ret = lm_clientlist_find(cl, addr, &cli);
    if(ret == 0 && cli != NULL) {
        cli->active = CLIENT_DEAD;
        CLIENT_TERM_PROCESS(cli, ret);
        CLIENT_RECOVER_RESOURCE(cli, ret);
        CLIENT_RECOVER_PUB_RIGHT(cli);
        *ppc = cli;
    }
    return ret;
}


int lm_clientlist_find_or_create(clientlist_t *cl, struct sockaddr_un *addr, client_t **ppc)
{
    int ret;
    int newslot = 0;
    uint64_t hval;
    size_t i;
    clientlist_t* ncur = NULL;
    client_t* ncli = NULL;
    client_t* cli = NULL;
    clientlist_t *cur = cl;
    socklen_t addr_len = SUN_LEN(addr);

    eal_spin_lock(&cl->lock);
    do {

        /* Find an empty slot, may use it after */
        if(cur->count < CLIENT_COUNT && ncli == NULL) {
            ncur = cur;
            ncli = &ncur->cli[ncur->count];
            newslot = 1;
        }

        for(i=0; i<cur->count; ++i) {
            if(cur->cli[i].addr_len == addr_len &&
                    memcmp(&(cur->cli[i].addr), addr, addr_len) == 0 &&
                    cur->cli[i].active == CLIENT_ALIVE ) {
                cli = &cur->cli[i];
                /* Find it */
                break;
            }
            /* Find dead slot, may use it after */
            if(cur->cli[i].active == CLIENT_DEAD) {
                ncur = cur;
                ncli = &cur->cli[i];
                newslot = 0;
            }
        }

        /* Does not exist */
        if(cli == NULL && cur->next == NULL) {
            /* No empty slot, so create a new one */
            if(ncli == NULL) {
                cur->next = lm_clientlist_create();
                ncur = cur->next;
                ncli = ncur->cli;
                newslot = 1;
            }

            /* Init client:IC */
            memset(ncli, 0, sizeof(client_t));
            memcpy(&ncli->addr, addr, addr_len);
            ncli->addr_len = addr_len;


            /* IC1: Create board shared memory */
            hval = eal_hash64_fnv1a(addr, addr_len);
            eal_shm_hash_name(hval, ncli->board.name);
            ncli->board.size = BOARD_SHMSIZE;
            ret = eal_shm_create(&ncli->board);
            if(ret == 0) {
                /* IC2: Create private semaphore */
                ret = eal_eventp_init((evtfd_t)ncli->board.addr);
                if(ret == 0) {
                    /* IC3: Create public semaphore */
                    eal_event_hash_name(hval, ncli->event.name);
                    ret = eal_event_create(&ncli->event);
                    if(ret == 0) {
                        if(newslot == 1) {
                            ++ncur->count;
                        }
                        ncli->active = CLIENT_ALIVE;
                        cli = ncli;
                    } else {
                        lmice_error_log("IC3 create event[%s] failed[%d]", ncli->event.name, ret);
                        /* Clean IC2 evt*/
                        eal_eventp_close((evtfd_t)ncli->board.addr);
                        /* Clean IC1 shm*/
                        eal_shm_destroy(&ncli->board);
                    }
                } else {
                    lmice_error_log("IC2 create event failed[%d]", ret);
                    /* Clean IC1 shm*/
                    eal_shm_destroy(&ncli->board);
                }
            } else {
                lmice_error_log("IC1 create client board shm[%s] failed[%d]", ncli->board.name, ret);
            }


            break;
        }

        cur = cur->next;
    } while(cur != NULL && cli == NULL);
    eal_spin_unlock(&cl->lock);

    *ppc = cli;
    return ret;
}


int lm_clientlist_find(clientlist_t *sl, struct sockaddr_un *addr, client_t **ppc)
{
    client_t * ps = NULL;
    clientlist_t* cur = sl;
    size_t i;
    socklen_t addr_len = SUN_LEN(addr);

    eal_spin_lock(&sl->lock);
    do {
        for(i=0; i<cur->count; ++i) {
            if(cur->cli[i].addr_len == addr_len &&
                    memcmp(&(cur->cli[i].addr), addr, addr_len) == 0) {

                if(cur->cli[i].active == CLIENT_ALIVE)
                    ps = &cur->cli[i];
                /* Find it */
                break;
            }
        }

        cur = cur->next;
    }while(cur != NULL && ps == NULL);
    eal_spin_unlock(&sl->lock);

    *ppc = ps;

    return 0;
}

int lm_clientlist_find_pid(clientlist_t *sl, pid_t pid, client_t **ppc)
{
    client_t * ps = NULL;
    clientlist_t* cur = sl;
    size_t i;

    eal_spin_lock(&sl->lock);
    do {
        for(i=0; i<cur->count; ++i) {
            if(cur->cli[i].pid == pid) {

                if(cur->cli[i].active == CLIENT_ALIVE)
                    ps = &cur->cli[i];
                /* Find it */
                break;
            }
        }

        cur = cur->next;
    }while(cur != NULL && ps == NULL);
    eal_spin_unlock(&sl->lock);

    *ppc = ps;

    return 0;

}


int lm_client_pub(client_t *cli, pubsub_shm_t *ps, const char *symbol)
{
    int ret;
    symbol_shm_t* sym = NULL;
    ret = lm_client_find_or_create(cli, ps, symbol, &sym);
    if(ret == 0 && sym) {
        sym->type |= SHM_PUB_TYPE;
    }
    return ret;
}


int lm_client_sub(client_t *cli, pubsub_shm_t *ps, const char *symbol)
{
    int ret;
    symbol_shm_t* sym = NULL;
    ret = lm_client_find_or_create(cli, ps, symbol, &sym);
    if(ret == 0 && sym) {
        sym->type |= SHM_SUB_TYPE;
    }
    return ret;
}


int lm_client_unpub(client_t *cli, pubsub_shm_t *ps, const char *symbol)
{
    symbol_shm_t* sym = NULL;
    size_t i;
    eal_spin_lock(&cli->lock);
    for(i=0; i<cli->count; ++i) {
        sym = &cli->symshm[i];
        if( (ps && ps->hval == sym->ps->hval) ||
            (!ps && memcmp(symbol, sym->symbol, SYMBOL_LENGTH) == 0) ) {
            if(sym->type & SHM_PUB_TYPE) {
                sym->ps->type &= (~SHM_PUB_TYPE);
                sym->type &= (~SHM_PUB_TYPE);
            }

            /* Recover symbol */
            if(sym->type == 0) {
                memmove(sym, sym+1, (cli->count-i-1)*sizeof(symbol_shm_t) );
                --cli->count;
            }
            break;
        }
    }/* end-for:cli->count */
    eal_spin_unlock(&cli->lock);
    return 0;
}


int lm_client_unsub(client_t *cli, pubsub_shm_t *ps, const char *symbol)
{
    symbol_shm_t* sym = NULL;
    size_t i;

    (void)ps;

    eal_spin_lock(&cli->lock);
    for(i=0; i<cli->count; ++i) {
        sym = &cli->symshm[i];
        if( (ps && ps->hval == sym->ps->hval) ||
            (!ps && memcmp(symbol, sym->symbol, SYMBOL_LENGTH) == 0) ) {
            if(sym->type & SHM_SUB_TYPE) {
                sym->type &= SHM_PUB_TYPE;
            }

            /* Recover symbol */
            if(sym->type == 0) {
                memmove(sym, sym+1, (cli->count-i-1)*sizeof(symbol_shm_t) );
                --cli->count;
            }
            break;
        }
    }/* end-for:cli->count */
    eal_spin_unlock(&cli->lock);
    return 0;
}


int lm_client_find_or_create(client_t *cli, pubsub_shm_t *ps, const char *symbol, symbol_shm_t **pps)
{
    int ret = 0;
    symbol_shm_t* sym = NULL;
    uint64_t hval;
    size_t i;
    hval = eal_hash64_fnv1a(symbol, SYMBOL_LENGTH);

    eal_spin_lock(&cli->lock);
    for(i=0; i<cli->count; ++i) {
        if( ps == cli->symshm[i].ps ) {
            sym = &cli->symshm[i];
            break;
        }
    }/* end-for:cli->count */
    if(sym == NULL) {
        if(cli->count < CLIENT_SPCNT) {
            sym = &cli->symshm[cli->count];
            memcpy(sym->symbol, symbol, SYMBOL_LENGTH);
            sym->type = 0;
            sym->ps = ps;
            ++cli->count;
        } else {
            ret = 1;
        }
    }
    eal_spin_unlock(&cli->lock);

    *pps = sym;
    return ret;

}


int lm_client_find(client_t *cli, pubsub_shm_t *ps, const char *symbol, symbol_shm_t **pps)
{
    int ret = 0;
    symbol_shm_t* sym = NULL;
    size_t i;
    (void)symbol;

    eal_spin_lock(&cli->lock);
    for(i=0; i<cli->count; ++i) {
        if( ps == cli->symshm[i].ps ) {
            sym = &cli->symshm[i];
            break;
        }
    }/* end-for:cli->count */
    eal_spin_unlock(&cli->lock);

    *pps = sym;
    return ret;
}


int lm_clientlist_maintain(clientlist_t *cl)
{
    int ret;
    int err;
    uint32_t i;
    clientlist_t *cur = cl;
    ret = eal_spin_trylock(&cl->lock);
    if(ret != 0)
        return 0;
    do {
        for(i= 0; i< cur->count; ++i) {
            client_t* cli = &cur->cli[i];
            if(cli->active == CLIENT_DEAD)
                continue;
            ret = kill(cli->pid, 0);
            if(ret != 0) { /* the process is not exist, so kill failed */
                size_t j;
                err = errno;
                lmice_critical_log("process[%u] open failed[%d]\n", cli->pid, err);
                lmice_shm_t *shm = &cli->board;
                lmice_event_t *event = &cli->event;
                ret = eal_shm_destroy(shm);
                lmice_critical_log("process[%u] remove shm[%s] as [%d]\n", cli->pid, shm->name, ret);
                ret = eal_event_destroy(event);
                lmice_critical_log("process[%u] remove evt[%s] as [%d]\n", cli->pid, event->name, ret);

                cli->active = CLIENT_DEAD;

                for(j=0; j< cli->count; ++j) {
                    symbol_shm_t *ss = &cli->symshm[j];
                    pubsub_shm_t *ps = ss->ps;
                    if(ss->type & SHM_PUB_TYPE) {
                        ps->type &= (~SHM_PUB_TYPE);
                    }
                }
                lmice_critical_log("process[%u] remove pub state\n", cli->pid);
            }
        }
        cur = cur->next;
    } while(cur != NULL);
    eal_spin_unlock(&cl->lock);
    return 0;
}


