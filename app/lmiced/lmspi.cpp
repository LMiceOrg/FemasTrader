#include "lmspi.h"
#include "fmspi.h"
#include "udss.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>


#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>

#include "lmice_trace.h"

#include "lmice_eal_bson.h"

#include "lmice_eal_shm.h"
#include "lmice_eal_event.h"
#include "lmice_eal_hash.h"
#include "lmice_eal_spinlock.h"

//#define EXCHANGE_ID "SHFE"

/** dir: 0:buy  1:sell
 * return: requestId
 *
 *  开 平:后台维护
 * 增加一个字段
 * （time，order, volumn)组合
 */
//int order(const char* symbol, int dir, double price, int num);

/**
 * @brief cancel
 * @param requestId: order return requestId
 * @param sysId: system return Id
 * return requestId
 */
//int cancel(int requestId, int sysId = 0);
/* order tracker */
//    int get_order(const char* order_id, struct order_t * order);
/* P & L tracker */
/* position tracker */
//private:
//	void logging_bson_order( void *ptrOrd );
//	void logging_bson_cancel( void *ptrOrd );

typedef void(*proto_callback)(const char* symbol, const void* addr, int size);
struct spi_shm_t{
    int type;
    uint64_t hval;
    char symbol[SYMBOL_LENGTH];
    lmice_shm_t shm;
    proto_callback callback;
    csymbol_callback pcall;
    void* udata;

};

struct worker_item_s {
    CLMSpi *pthis;
    csymbol_callback pcall;
    proto_callback callback;
    char symbol[SYMBOL_LENGTH];
    const void* addr;
    int size;

};
typedef struct worker_item_s worker_item_t;

struct worker_pool_t {
    int64_t lock;
    evtfd_t count;
    worker_item_t item[128];
};

struct spi_private {
    CLMSpi *pthis;

    pthread_t pt;
    volatile int quit_flag;
    lmice_event_t event;
    lmice_shm_t server;
    lmice_shm_t board;
    uint32_t shmcount;
    spi_shm_t shmlist[SHMLIST_COUNT];
    csymbol_callback pcall;
    proto_callback callback;
    void* udata;
    pthread_t ppool[4];
    char name[SYMBOL_LENGTH];
    uds_msg sid;

};

#define SOCK_FILE "/var/run/lmiced.socket"

#define OPT_LOG_DEBUG 1

#define UPDATE_INFO(info) do {    \
        time(&((info)->tm)); \
        get_system_time(&((info)->systime));   \
        (info)->loglevel = 0;   \
        (info)->pid = getpid(); \
        (info)->tid = eal_gettid(); \
    } while(0)

static volatile int quit_flag = 0;

static int spi_shm_open(lmice_shm_t* shm, const char* name, int name_size, int shm_size) {
    int ret;
    uint64_t hval;

    eal_shm_zero(shm);
    hval = eal_hash64_fnv1a(name, name_size);
    eal_shm_hash_name(hval, shm->name);
    shm->size = shm_size; /* 4K bytes*/
    ret = eal_shm_open_readwrite(shm);
    return ret;
}

//sig_t sigint_handler = NULL;
sig_t sigterm_handler = NULL;

void spi_signal_handler(int sig) {
    if(sig == SIGTERM ||sig == SIGINT) {
        quit_flag = 1;
        if(sigterm_handler) {
            sigterm_handler(sig);
        }
    }
}


void* spi_thread(void* priv) {
    spi_private* p = (spi_private*) priv;
    uint32_t cnt;
    sub_detail_t* symlist = new sub_detail_t[CLIENT_SPCNT];
    memset(symlist, 0, sizeof(sub_detail_t)*CLIENT_SPCNT);
    for(;;) {
        int ret = eal_event_wait_timed(p->event.fd, 500);
        if(ret == -1) {
            /* Timed out or interrupted */
        } else if(ret == 0) {
            /* Event fired */
            lmice_sub_data_t* dt = (lmice_sub_data_t*)((char*)p->board.addr + CLIENT_SUBPOS);

            /* Copy event */
            eal_spin_lock(&dt->lock);

            cnt = dt->count;
            if(cnt <= CLIENT_SPCNT) {
                memcpy(symlist, dt->sub, cnt*sizeof(sub_detail_t));
            } else {
                cnt = 0;
            }
            dt->count = 0;

            eal_spin_unlock(&dt->lock);

            //lmice_critical_print("Got %u message. %lu, %u\n", cnt, symlist->hval, j<p->shmcount);

            /* Callback event */
            for(size_t i=0; i<cnt; ++i) {
                sub_detail_t* sd = symlist +i;
                for(size_t j=0; j<p->shmcount; ++j) {
                    spi_shm_t* ps = &p->shmlist[j];
                    lmice_shm_t* shm = &ps->shm;
                    const char* addr = (const char*)shm->addr+sd->pos+sizeof(lmice_data_detail_t);
                    if(sd->hval == ps->hval && ps->type & CLIENT_SUBSYM) {
                        if(shm->addr == 0) {
                            /* open shm */
                            eal_shm_open_readwrite(shm);
                        }
                        if(shm->addr) {
                            if(ps->callback) {
                                ps->callback(ps->symbol, addr, sd->size);
                            }
                            if(p->pthis && ps->pcall) {
                                (p->pthis->*ps->pcall)(ps->symbol, addr, sd->size);
                            }
                            if(p->callback) {
                                p->callback(ps->symbol, addr, sd->size);
                            }
                            if(p->pthis && p->pcall) {
                                (p->pthis->*p->pcall)(ps->symbol, addr, sd->size);
                            }
                            break;
                        } else {
                            lmice_error_print("Can't open message[%s].\n", ps->symbol);
                        }
                    }
                } /* for-j: shmcount */
            }/* for-i:cnt */
        } /* else-if ret */

        if(quit_flag == 1) {
            lmice_critical_print("Quit[Signal] worker process\n");
            break;
        }
        if(p->quit_flag == 1) {
            lmice_critical_print("Quit worker process\n");
            break;
        }
    }/* end-for: ;;*/

    delete[] symlist;
    return NULL;
}

CLMSpi::CLMSpi(const char *name, int poolsize)
{
    int ret;
    // create private resource
    spi_private* p = new spi_private;
    memset(p, 0, sizeof(spi_private));
    m_priv = (void*) p;
    uds_msg* sid = &p->sid;    
    memcpy(p->name, name, strlen(name)>SYMBOL_LENGTH-1?SYMBOL_LENGTH-1:strlen(name));

    //Init uds client
    ret = init_uds_client(SOCK_FILE, sid);
    if(ret != 0) {
        lmice_error_print("Can't init UDS client[%d].\n", ret);
        exit(ret);
    }
    lmice_register_t *reg = (lmice_register_t*)sid->data;
    lmice_trace_info_t* pinfo = &reg->info;
    UPDATE_INFO(pinfo);

    // Register signal handler
    signal(SIGINT, spi_signal_handler);
    /*signal(SIGCHLD,SIG_IGN);  ignore child */
    /* signal(SIGTSTP,SIG_IGN);  ignore tty signals */
    signal(SIGTERM,spi_signal_handler); /* catch kill signal */

    ret = spi_shm_open(&p->server, BOARD_NAME, sizeof(BOARD_NAME)-1, CLIENT_BOARD);
    if(ret != 0) {
        lmice_error_print("Can't access LMICE daemon[%d].\n", ret);
        finit_uds_msg(sid);
        exit(ret);
    }

    //Register client
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_REGEVT);
    lmice_register_data_t* pd = (lmice_register_data_t*)((char*)p->server.addr+SERVER_REGPOS);

    //Try awake lmiced with event
    bool sended = false;
    ret = eal_spin_trylock(&pd->lock);
    if( ret == 0) {
        if(pd->count < REGLIST_LENGTH) {
            lmice_register_detail_t *dt = pd->reg + pd->count;
            dt->tid= eal_gettid();
            dt->pid = getpid();
            dt->type = EM_LMICE_REGCLIENT_TYPE;
            memcpy(dt->symbol,p->name, SYMBOL_LENGTH);
            memcpy(&dt->un, &sid->local_un, sizeof(dt->un));
            ++pd->count;
            sended = true;
        }
        eal_spin_unlock(&pd->lock);
    }

    if(sended) {
        ret = eal_event_awake( efd );
    } else {
        /* Fall back to use UDS */
        pinfo->type = EM_LMICE_REGCLIENT_TYPE;
        sid->size = sizeof(lmice_register_t);
        memcpy(reg->symbol,p->name, SYMBOL_LENGTH);
        ret = send_uds_msg(sid);
    }

    if(ret == 0 ) {
        lmice_critical_print("Register model[%s] in[%s]\n", name, sid->local_un.sun_path);
        usleep(10000);
    } else {
        int err = errno;
        lmice_error_print("Register model[%s] failed[%d].\n", name, err);
        finit_uds_msg(sid);
        exit(ret);
    }

    //Init spi by local_un
    uint64_t hval;
    hval = eal_hash64_fnv1a(&sid->local_un, SUN_LEN(&sid->local_un));

    eal_shm_hash_name(hval, p->board.name);
    eal_event_hash_name(hval, p->event.name);
    for(size_t i=0; i<3; ++i) {
        ret = eal_shm_open_readwrite(&p->board);
        if(ret != 0) {
            lmice_error_print("Open shm[%s] failed[%d]\n", p->board.name, ret);
            usleep(10000);
            continue;
        }
        ret = eal_event_open(&p->event);
        if(ret != 0) {
            lmice_error_print("Open evt[%s] failed[%d]\n", p->event.name, ret);
            usleep(10000);
            continue;
        }
    }

    if(ret != 0) {
        lmice_error_print("Init model[%s] resource failed[%d].\n", name, ret);
        finit_uds_msg(sid);
        exit(ret);
    }

    /* running and create event thread */
    logging("LMice client[ %s ] running %d:%d...", p->name, getuid(), getpid());
    pthread_create(&p->pt, NULL, spi_thread, m_priv);

}


CLMSpi::~CLMSpi()
{
    spi_private* p = (spi_private*)m_priv;
    uds_msg *sid = &p->sid;
    //Register client
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_REGEVT);
    lmice_register_data_t* pd = (lmice_register_data_t*)((char*)p->server.addr+SERVER_REGPOS);

    //Try awake lmiced with event
    bool sended = false;
    int ret;
    ret = eal_spin_trylock(&pd->lock);
    if( ret == 0) {
        if(pd->count < REGLIST_LENGTH) {
            lmice_register_detail_t *dt = pd->reg + pd->count;
            dt->tid= eal_gettid();
            dt->pid = getpid();
            dt->type = EM_LMICE_UNREGCLIENT_TYPE;
            memcpy(&dt->un, &sid->local_un, sizeof(dt->un));
            ++pd->count;
            sended = true;
        }
        eal_spin_unlock(&pd->lock);
    }

    if(sended) {
        ret = eal_event_awake( efd );
    } else {
        /* Fall back to use UDS */
        lmice_register_t *reg = (lmice_register_t*)sid->data;
        lmice_trace_info_t* pinfo = &reg->info;
        UPDATE_INFO(pinfo);
        pinfo->type = EM_LMICE_UNREGCLIENT_TYPE;
        sid->size = sizeof(lmice_register_t);
        ret = send_uds_msg(sid);
    }

    logging("LMice client[ %s ] stopped %d:%d.\n\n", p->name, getuid(), getpid());

    p->quit_flag = 1;
    if(p->pt != 0) {
        pthread_join(p->pt, NULL);
    }
    finit_uds_msg(&p->sid);

    eal_event_close(p->event.fd);
    eal_shm_close(p->server.fd, p->server.addr);
    eal_shm_close(p->board.fd, p->board.addr);
    delete p;
}

void CLMSpi::register_signal(sig_t sigfunc) {
    sigterm_handler = sigfunc;
}

void CLMSpi::logging(const char* format, ...) {
    va_list argptr;
    spi_private* p = (spi_private*)m_priv;
    uds_msg* sid = &p->sid;
    lmice_trace_info_t* info = (lmice_trace_info_t*)sid->data;

    UPDATE_INFO(info);
    info->type = LMICE_TRACE_TYPE;

    va_start(argptr, format);
    sid->size = sizeof(lmice_trace_info_t) + vsprintf(sid->data+sizeof(lmice_trace_info_t), format, argptr);
    va_end(argptr);

    send_uds_msg(sid);
}

//void CLMSpi::logging_bson_order( void *ptrOrd )
//{
//	if( NULL == ptrOrd )
//	{
//		return;
//	}
	
//	CUstpFtdcInputOrderField *ord = (CUstpFtdcInputOrderField *)ptrOrd;
//    lmice_trace_bson_info_t *pinfo = (lmice_trace_bson_info_t *)sid->data;
//	EalBson bson;
//    time(&pinfo->tm);
//    get_system_time(&pinfo->systime);
//	strcpy(pinfo->model_name, m_name.c_str());
//    pinfo->type = EMZ_LMICE_TRACEZ_BSON_TYPE;

//	bson.AppendTimeT("time", pinfo->tm);
//	bson.AppendSymbol("BrokerID", ord->BrokerID);
//	bson.AppendSymbol("ExchangeID", ord->ExchangeID);
//	bson.AppendSymbol("OrderSysID", ord->OrderSysID);
//	bson.AppendSymbol("InvestorID", ord->InvestorID);
//	bson.AppendSymbol("UserID", ord->UserID);
//	bson.AppendSymbol("InstrumentID", ord->InstrumentID);
//	bson.AppendSymbol("UserOrderLocalID", ord->UserOrderLocalID);
//	bson.AppendFlag("OrderPriceType", ord->OrderPriceType);
//	bson.AppendFlag("Direction", ord->Direction);
//	bson.AppendFlag("OffsetFlag", ord->OffsetFlag);
//	bson.AppendFlag("HedgeFlag", ord->HedgeFlag);
//	bson.AppendDouble("LimitPrice",ord->LimitPrice);
//	bson.AppendInt64("Volume",ord->Volume);
//	bson.AppendFlag("TimeCondition", ord->TimeCondition);
//	bson.AppendSymbol("GTDDate", ord->GTDDate);
//	bson.AppendFlag("VolumeCondition", ord->VolumeCondition);
//	bson.AppendInt64("MinVolume",ord->MinVolume);
//	bson.AppendDouble("StopPrice",ord->StopPrice);
//	bson.AppendFlag("ForceCloseReason", ord->ForceCloseReason);
//	bson.AppendInt64("MinVolume",ord->MinVolume);
//	bson.AppendInt64("IsAutoSuspend",ord->IsAutoSuspend);
//	bson.AppendSymbol("BusinessUnit", ord->BusinessUnit);
//	bson.AppendSymbol("UserCustom", ord->UserCustom);
//	bson.AppendInt64("BusinessLocalID",ord->BusinessLocalID);
//	bson.AppendSymbol("ActionDay", ord->ActionDay);
//	memcpy( sid->data+sizeof(m_info), bson.GetBsonData(), bson.GetLen());

//    send_uds_msg(sid);
//}

//void CLMSpi::logging_bson_cancel( void *ptrOrd )
//{
//	if( NULL == ptrOrd )
//	{
//		return;
//	}

//	CUstpFtdcOrderActionField *ord = (CUstpFtdcOrderActionField *)ptrOrd;
	
//    lmice_trace_bson_info_t *pinfo = (lmice_trace_bson_info_t *)sid->data;
//	EalBson bson;
//    get_system_time(&pinfo->systime);
//	strcpy(pinfo->model_name, m_name.c_str());
//    pinfo->type = EMZ_LMICE_TRACEZ_BSON_TYPE;

//	bson.AppendTimeT("time", pinfo->tm);
//	bson.AppendSymbol("BrokerID", ord->BrokerID);
//	bson.AppendSymbol("ExchangeID", ord->ExchangeID);
//	bson.AppendSymbol("OrderSysID", ord->OrderSysID);
//	bson.AppendSymbol("InvestorID", ord->InvestorID);
//	bson.AppendSymbol("UserID", ord->UserID);
//	bson.AppendSymbol("UserOrderLocalID", ord->UserOrderLocalID);
//	bson.AppendSymbol("UserOrderActionLocalID", ord->UserOrderActionLocalID);
//	bson.AppendFlag("ActionFlag", ord->ActionFlag);
//	bson.AppendDouble("LimitPrice",ord->LimitPrice);
//	bson.AppendInt64("VolumeChange",ord->VolumeChange);
//	bson.AppendInt64("BusinessLocalID",ord->BusinessLocalID);
//	memcpy( sid->data+sizeof(m_info), bson.GetBsonData(), bson.GetLen());

//    send_uds_msg(sid);
//}

static int code_convert(const char *from_charset,const char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
        iconv_t cd;
        char **pin = &inbuf;
        char **pout = &outbuf;

        cd = iconv_open(to_charset,from_charset);
        if (cd==0)
                return -1;
        memset(outbuf,0,outlen);
        if (iconv(cd,pin,&inlen,pout,&outlen) == (size_t)-1)
                return -1;
        iconv_close(cd);
        return 0;
}

std::string CLMSpi::gbktoutf8( char *pgbk)
{
    size_t inlen = strlen(pgbk);
    char outbuf[512] ={0};
    size_t outlen = 512;
    code_convert("gbk","utf-8",pgbk,inlen,(char*)outbuf,outlen);

    std::string utf8;
    utf8.insert(utf8.begin(), outbuf, outbuf+strlen(outbuf));
    return utf8;

}

void CLMSpi::subscribe(const char *symbol)
{
    int ret;
    spi_private* p =(spi_private*)m_priv;
    lmice_symbol_data_t* pd = (lmice_symbol_data_t*)((char*)p->server.addr+SERVER_SYMPOS);
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_SYMEVT);


    if(p->shmcount >= CLIENT_SPCNT) {
        lmice_error_print("Sub/pub resource is full\n");
        return;
    }
    if(strlen(symbol) >= SYMBOL_LENGTH) {
        lmice_error_print("Sub symbol is too long.\n");
        return;
    }
    char sym[SYMBOL_LENGTH] = {0};
    uint64_t hval;
    strcpy(sym, symbol);
    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);

    spi_shm_t*ps = NULL;
    lmice_shm_t*shm;
    for(size_t i=0; i< p->shmcount; ++i) {
        ps = &p->shmlist[i];
        shm = &ps->shm;
        if(ps->hval == hval) {
            if(ps->type & SHM_SUB_TYPE) {
                lmice_critical_print("Already subscribed resource[%s]\n", sym);
                return;
            } else {
                ps->type |= SHM_SUB_TYPE;
                break;
            }
        }
        ps = NULL;

    }

    if(!ps && p->shmcount < SHMLIST_COUNT) {
        ps = &p->shmlist[p->shmcount];
        shm = &ps->shm;
        ps->type |= SHM_SUB_TYPE;
        ps->hval = hval;
        memcpy(ps->symbol, sym, SYMBOL_LENGTH);
        memset(shm, 0, sizeof(lmice_shm_t) );
        eal_shm_hash_name(hval, shm->name);
        ++p->shmcount;
    }

    //Try awake lmiced with event
    bool sended = false;
    ret = eal_spin_trylock(&pd->lock);
    if( ret == 0) {
        if(pd->count < SYMLIST_LENGTH) {
            lmice_symbol_detail_t *dt = pd->sym + pd->count;
            dt->hval= hval;
            dt->pid = getpid();
            dt->type = EM_LMICE_SUB_TYPE;
            memcpy(dt->symbol, sym, SYMBOL_LENGTH);
            ++pd->count;
            sended = true;
        }
        eal_spin_unlock(&pd->lock);
    }

    if(sended) {
        ret = eal_event_awake( efd );
    } else {
        /* Fall back to use UDS */
        uds_msg* sid = &p->sid;
        lmice_sub_t *psub = (lmice_sub_t *)sid->data;
        lmice_trace_info_t *pinfo = &psub->info;
        UPDATE_INFO(pinfo);
        pinfo->type = EM_LMICE_SUB_TYPE;
        sid->size = sizeof(lmice_sub_t);
        memcpy(psub->symbol, sym, SYMBOL_LENGTH);

        send_uds_msg(sid);
    }


}

void CLMSpi::unsubscribe(const char *symbol)
{
    int ret;
    spi_private* p =(spi_private*)m_priv;
    uds_msg* sid = &p->sid;
    lmice_symbol_data_t* pd = (lmice_symbol_data_t*)((char*)p->server.addr+SERVER_SYMPOS);
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_SYMEVT);

    if(p->shmcount == 0) {
        lmice_error_print("Sub/pub resource is empty.\n");
        return;
    }
    if(strlen(symbol) >= SYMBOL_LENGTH) {
        lmice_error_print("Sub symbol is too long.\n");
        return;
    }
    char sym[SYMBOL_LENGTH] = {0};
    char name[SYMBOL_LENGTH] ={0};
    uint64_t hval;
    strcpy(sym, symbol);
    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);
    eal_shm_hash_name(hval, name);

    spi_shm_t*ps = NULL;
    lmice_shm_t*shm;
    for(size_t i=0; i< p->shmcount; ++i) {
        ps = &p->shmlist[i];
        shm = &ps->shm;
        if(ps->type & SHM_SUB_TYPE && ps->hval == hval) {
            ps->type &= (~SHM_SUB_TYPE);
            if(ps->type == 0) {
                if(shm->fd != 0 && shm->addr != 0) {
                    /* Close shm */
                    eal_shm_close(shm->fd, shm->addr);
                }
                memmove(ps, ps+1, (p->shmcount-i-1)*sizeof(spi_shm_t));
                --p->shmcount;
            }
            break;

        }

        ps = NULL;

    }

    if(!ps)
        return;

    //Try awake lmiced with event
    bool sended = false;
    ret = eal_spin_trylock(&pd->lock);
    if( ret == 0) {
        if(pd->count < SYMLIST_LENGTH) {
            lmice_symbol_detail_t *dt = pd->sym + pd->count;
            dt->hval= hval;
            dt->pid = getpid();
            dt->type = EM_LMICE_UNSUB_TYPE;
            memcpy(dt->symbol, sym, SYMBOL_LENGTH);
            ++pd->count;
            sended = true;
        }
        eal_spin_unlock(&pd->lock);
    }

    if(sended) {
        ret = eal_event_awake( efd );
    } else {
        /* Fall back to use UDS */
        sid->size = sizeof(lmice_unsub_t);
        lmice_unsub_t *psub = (lmice_unsub_t *)sid->data;
        lmice_trace_info_t *pinfo = &psub->info;

        time(&pinfo->tm);
        get_system_time(&pinfo->systime);
        pinfo->type = EM_LMICE_UNSUB_TYPE;
        memcpy(psub->symbol, sym, SYMBOL_LENGTH);

        send_uds_msg(sid);
    }
}

void CLMSpi::publish(const char *symbol)
{
    int ret;
    spi_private* p =(spi_private*)m_priv;
    uds_msg *sid = &p->sid;
    lmice_symbol_data_t* pd = (lmice_symbol_data_t*)((char*)p->server.addr+SERVER_SYMPOS);
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_SYMEVT);

    if(p->shmcount >= CLIENT_SPCNT) {
        lmice_error_print("Sub/pub resource is full\n");
        return;
    }
    if(strlen(symbol) >= SYMBOL_LENGTH) {
        lmice_error_print("Pub symbol is too long.\n");
        return;
    }
    char sym[SYMBOL_LENGTH] = {0};

    uint64_t hval;
    strcpy(sym, symbol);
    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);


    spi_shm_t*ps = NULL;
    lmice_shm_t*shm;
    for(size_t i=0; i< p->shmcount; ++i) {
        ps = &p->shmlist[i];
        shm = &ps->shm;
        if(ps->hval == hval) {
            if(ps->type & SHM_PUB_TYPE) {
                lmice_critical_print("Already published resource[%s]\n", symbol);
            } else {
                ps->type |= SHM_PUB_TYPE;
            }
            break;
        }
        ps = NULL;

    }
    if(!ps && p->shmcount < SHMLIST_COUNT) {
        ps = &p->shmlist[p->shmcount];
        shm = &ps->shm;
        ps->hval = hval;
        ps->type |= SHM_PUB_TYPE;
        memcpy(ps->symbol, sym, SYMBOL_LENGTH);
        memset(shm, 0, sizeof(lmice_shm_t) );
        eal_shm_hash_name(hval, shm->name);
        ++p->shmcount;
    }

    //Try awake lmiced with event
    bool sended = false;
    ret = eal_spin_trylock(&pd->lock);
    if( ret == 0) {
        if(pd->count < SYMLIST_LENGTH) {
            lmice_symbol_detail_t *dt = pd->sym + pd->count;
            dt->hval= hval;
            dt->pid = getpid();
            dt->type = EM_LMICE_PUB_TYPE;
            memcpy(dt->symbol, sym, SYMBOL_LENGTH);
            ++pd->count;
            sended = true;
        }
        eal_spin_unlock(&pd->lock);
    }

    if(sended) {
        ret = eal_event_awake( efd );
    } else {
        /* Fall back to use UDS */
        sid->size = sizeof(lmice_pub_t);
        lmice_pub_t *pp = (lmice_pub_t *)sid->data;
        lmice_trace_info_t *pinfo = &pp->info;
        UPDATE_INFO(pinfo);
        pinfo->type = EM_LMICE_PUB_TYPE;
        memcpy(pp->symbol, sym, SYMBOL_LENGTH);
        send_uds_msg(sid);
    }
}

void CLMSpi::unpublish(const char *symbol)
{
    int ret;
    spi_private* p =(spi_private*)m_priv;
    uds_msg* sid = &p->sid;
    lmice_symbol_data_t* pd = (lmice_symbol_data_t*)((char*)p->server.addr+SERVER_SYMPOS);
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_SYMEVT);

    if(p->shmcount == 0) {
        lmice_error_print("pub resource is empty.\n");
        return;
    }
    if(strlen(symbol) >= SYMBOL_LENGTH) {
        lmice_error_print("Ubpublish symbol is too long.\n");
        return;
    }
    char sym[SYMBOL_LENGTH] = {0};
    char name[SYMBOL_LENGTH] ={0};
    uint64_t hval;
    strcpy(sym, symbol);
    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);
    eal_shm_hash_name(hval, name);

    spi_shm_t*ps = NULL;
    lmice_shm_t*shm;
    for(size_t i=0; i< p->shmcount; ++i) {
        ps = &p->shmlist[i];
        shm = &ps->shm;
        if(ps->type & SHM_PUB_TYPE && ps->hval == hval) {
            ps->type &= (~SHM_PUB_TYPE);
            if(ps->type == 0) {
                if(shm->fd != 0 && shm->addr != 0) {
                    /* Close shm */
                    eal_shm_close(shm->fd, shm->addr);
                }
                memmove(ps, ps+1, (p->shmcount-i-1)*sizeof(spi_shm_t));
                --p->shmcount;
            }
            break;

        }

        ps = NULL;

    }

    if(!ps)
        return;

    //Try awake lmiced with event
    bool sended = false;
    ret = eal_spin_trylock(&pd->lock);
    if( ret == 0) {
        if(pd->count < SYMLIST_LENGTH) {
            lmice_symbol_detail_t *dt = pd->sym + pd->count;
            dt->hval= hval;
            dt->pid = getpid();
            dt->type = EM_LMICE_UNPUB_TYPE;
            memcpy(dt->symbol, sym, SYMBOL_LENGTH);
            ++pd->count;
            sended = true;
        }
        eal_spin_unlock(&pd->lock);
    }

    if(sended) {
        ret = eal_event_awake( efd );
    } else {
        /* Fall back to use UDS */
        sid->size = sizeof(lmice_unpub_t);
        lmice_unpub_t *pb = (lmice_unpub_t *)sid->data;
        lmice_trace_info_t *pinfo = &pb->info;

        UPDATE_INFO(pinfo);
        pinfo->type = EM_LMICE_UNPUB_TYPE;
        memcpy(pb->symbol, sym, SYMBOL_LENGTH);

        send_uds_msg(sid);
    }
}

//void CLMSpi::send(const char *symbol, const void *addr, int len)
//{
//    int ret;
//    spi_private* p =(spi_private*)m_priv;

//    if(p->shmcount >= CLIENT_SPCNT) {
//        lmice_error_print("Sub/pub resource is full\n");
//        return;
//    }
//    if(strlen(symbol) >= SYMBOL_LENGTH) {
//        lmice_error_print("Pub symbol is too long.\n");
//        return;
//    }
//    char sym[SYMBOL_LENGTH] = {0};
//    char name[SYMBOL_LENGTH] ={0};
//    uint64_t hval;
//    strcpy(sym, symbol);
//    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);
//    eal_shm_hash_name(hval, name);

//    spi_shm_t*ps = NULL;
//    lmice_shm_t*shm;
//    for(size_t i=0; i< p->shmcount; ++i) {
//        ps = &p->shmlist[i];
//        shm = &ps->shm;
//        if(ps->type & SHM_PUB_TYPE && ps->hval == hval) {
//            if(shm->fd == 0) {
//                ret =eal_shm_open_readwrite(shm);
//                if(ret != 0) {
//                    lmice_error_print("spi_send:Open shm[%s] failed[%d]\n", shm->name, ret);
//                    break;
//                }
//            }
//            // write data
//            lmice_data_detail_t* data = (lmice_data_detail_t*)shm->addr;
//            eal_spin_lock(&data->lock);
//            memcpy((char*)shm->addr+data->pos+sizeof(lmice_data_detail_t), addr, len);
//            //padding 8bytes aligned
//            data->pos += len + 8 - (len % 8);
//            eal_spin_unlock(&data->lock);
//            // Update lmiced
//            lmice_send_data_t* sd = (lmice_send_data_t*)sid->data;
//            sd->info.type = EM_LMICE_SEND_DATA;
//            sd->sub.pos = 0;
//            sd->sub.size = ret;
//            sd->sub.hval = hval;
//            send_uds_msg(sid);
//            break;
//        }
//        ps = NULL;

//    }
//}

void CLMSpi::send(const char *symbol, const void *addr, int len)
{
    int ret;
    spi_private* p =(spi_private*)m_priv;
    uds_msg *sid = &p->sid;
    lmice_pub_data_t* pub = (lmice_pub_data_t*)((char*)p->server.addr+SERVER_PUBPOS);
    evtfd_t efd = (evtfd_t)((char*)p->server.addr+SERVER_EVTPOS);

    if(!symbol) {
        lmice_error_print("Send symbol is null\n");
        return;
    }
    if(strlen(symbol) >= SYMBOL_LENGTH) {
        lmice_error_print("Send symbol is too long.\n");
        return;
    }

    if(len > SYMBOL_SHMSIZE /2) {
        lmice_error_print("Send len is too big.\n");
        return;
    }

    char sym[SYMBOL_LENGTH] = {0};
    char name[SYMBOL_LENGTH] ={0};
    uint64_t hval;
    strcpy(sym, symbol);
    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);
    eal_shm_hash_name(hval, name);

    spi_shm_t*ps = NULL;
    lmice_shm_t*shm;
    for(size_t i=0; i< p->shmcount; ++i) {
        ps = &p->shmlist[i];
        shm = &ps->shm;
        if(ps->type & CLIENT_PUBSYM && ps->hval == hval) {
            if(shm->fd == 0) {
                ret =eal_shm_open_readwrite(shm);
                if(ret != 0) {
                    lmice_error_print("spi_send:Open shm[%s] failed[%d]\n", shm->name, ret);
                    break;
                }
            }

            uint32_t pos;
            uint32_t ulen = ((len + 7)/8)*8; //padding 8bytes aligned
            lmice_data_detail_t* data = (lmice_data_detail_t*)shm->addr;
            // write data
            eal_spin_lock(&data->lock);
            //revise data write pos
            if(data->pos + len + sizeof(lmice_data_detail_t) >= SYMBOL_SHMSIZE) {
                data->pos = 0;
            }
            memcpy((char*)shm->addr+data->pos+sizeof(lmice_data_detail_t), addr, len);
            pos = data->pos;
            ++data->count;

            data->pos += ulen;

            eal_spin_unlock(&data->lock);

            //Try awake lmiced with event
            bool sended = false;
            ret = eal_spin_trylock(&pub->lock);
            if( ret == 0) {
                if(pub->count < PUBLIST_LENGTH) {
                    pub_detail_t *pd = pub->pub + pub->count;
                    pd->pos= pos;
                    pd->size =len;
                    pd->hval = hval;
                    ++pub->count;
                    sended = true;
                }
                eal_spin_unlock(&pub->lock);
            }

            if(sended) {
                ret = eal_event_awake( efd );
            } else {
                //Fallback to send the data by UDS
                // Update lmiced
                sid->size = sizeof(lmice_send_data_t);
                lmice_send_data_t* sd = (lmice_send_data_t*)sid->data;
                UPDATE_INFO(&sd->info);
                sd->info.type = EM_LMICE_SEND_DATA;
                sd->sub.pos = pos;
                sd->sub.size = len;
                sd->sub.hval = hval;
                send_uds_msg(sid);
                lmice_critical_print("Fallback to UDS send\n");
            }

            break;
        }//if-found
    }
}

//int CLMSpi::cancel(int requestId, int sysId)
//{
//    /*FIXME: autoincrease */
//    int req = 0;
//    //CTraderSpi* spi = static_cast<CTraderSpi*>(this);
//    CUstpFtdcOrderActionField ord;
//    memset(&ord,0, sizeof(ord));

//    //requestId = spi->GetRequestID();
//    ///交易所代码
//    memcpy(ord.ExchangeID, EXCHANGE_ID, sizeof(EXCHANGE_ID)-1 );
//    ///报单编号 优先级最高，填写系统返回单号，撤报单
//    if(sysId) {
//        sprintf(ord.OrderSysID, "%012d", sysId);
//    }

//    ///经纪公司编号
//    //memcpy(ord.BrokerID, spi->GetConf()->g_BrokerID, sizeof(ord.BrokerID) );
//    ///投资者编号
//    //memcpy(ord.InvestorID, spi->GetConf()->g_InvestorID, sizeof(ord.InvestorID) );
//    ///用户代码
//    //memcpy(ord.UserID, spi->GetConf()->g_UserID, sizeof(ord.UserID) );
//    ///本次撤单操作的本地编号
//    //TUstpFtdcUserOrderLocalIDType	UserOrderActionLocalID;
//    sprintf(ord.UserOrderActionLocalID, "%012d",req);

//    ///被撤订单的本地报单编号
////    TUstpFtdcUserOrderLocalIDType	UserOrderLocalID;
//    sprintf(ord.UserOrderLocalID, "%012d", requestId);
//    ///报单操作标志 ‘0’
////    TUstpFtdcActionFlagType	ActionFlag;
//    ord.ActionFlag =USTP_FTDC_AF_Delete;
//    /// 下面字段可空
//    ///价格
//    //TUstpFtdcPriceType	LimitPrice;
//    ///数量变化
//    //TUstpFtdcVolumeType	VolumeChange;
//    ///本地业务标识
//    //TUstpFtdcBusinessLocalIDType	BusinessLocalID;

//    /// 发出撤单操作
//    //spi->GetTrader()->ReqOrderAction(&ord, req);

//#ifdef OPT_LOG_DEBUG
//	int64_t systime = 0;
//	get_system_time(&systime);
//	EalBson bson;
//	bson.AppendInt64("time", systime);
//	bson.AppendSymbol("BrokerID", ord.BrokerID);
//	bson.AppendSymbol("ExchangeID", ord.ExchangeID);
//	bson.AppendSymbol("OrderSysID", ord.OrderSysID);
//	bson.AppendSymbol("InvestorID", ord.InvestorID);
//	bson.AppendSymbol("UserID", ord.UserID);
//	bson.AppendSymbol("UserOrderLocalID", ord.UserOrderLocalID);
//	bson.AppendSymbol("UserOrderActionLocalID", ord.UserOrderActionLocalID);
//	bson.AppendFlag("ActionFlag", ord.ActionFlag);
//	bson.AppendDouble("LimitPrice",ord.LimitPrice);
//	bson.AppendInt64("VolumeChange",ord.VolumeChange);
//	bson.AppendInt64("BusinessLocalID",ord.BusinessLocalID);

//	const char *strJson = bson.GetJsonData();

//	logging("[future opt] send ( cancel ), content: %s", strJson);

//	bson.FreeJsonData();
//#endif

//	//logging_bson_cancel( (void *)&ord );

//    return req;

//}

static uint64_t code_from_symbol(const char* symbol) {
    char sym[SYMBOL_LENGTH] = {0};
    uint64_t hval;
    strcpy(sym, symbol);
    hval = eal_hash64_fnv1a(sym, SYMBOL_LENGTH);
    return hval;
}

int CLMSpi::register_callback(symbol_callback func, const char* symbol)
{
    spi_private* p =(spi_private*)m_priv;
    if(symbol == NULL) {
        p->callback = func;
    } else {
        if(strlen(symbol) >= SYMBOL_LENGTH) {
            lmice_error_print("Symbol is too long.\n");
            return -1;
        }
        uint64_t hval = code_from_symbol(symbol);
        for(size_t j=0; j<p->shmcount; ++j) {
            spi_shm_t* ps = &p->shmlist[j];
            if(hval == ps->hval && ps->type & CLIENT_SUBSYM) {
                ps->callback = func;
                break;
            }
        } /* for-j: shmcount */

    }

    return 0;
}

int CLMSpi::register_cb(csymbol_callback func, const char *symbol)
{
    spi_private* p =(spi_private*)m_priv;
    if(symbol == NULL) {
        p->pthis = this;
        p->pcall = func;
    } else {
        if(strlen(symbol) >= SYMBOL_LENGTH) {
            lmice_error_print("Symbol is too long.\n");
            return -1;
        }
        uint64_t hval = code_from_symbol(symbol);
        for(size_t j=0; j<p->shmcount; ++j) {
            spi_shm_t* ps = &p->shmlist[j];
            if(hval == ps->hval && ps->type & CLIENT_SUBSYM) {
                p->pthis  = this;
                ps->pcall = func;
                break;
            }
        } /* for-j: shmcount */

    }

    return 0;
}


int CLMSpi::join()
{
    int ret = 0;
    spi_private* p =(spi_private*)m_priv;
    ret = pthread_join(p->pt, NULL);
    p->pt = 0;
    return ret;
}

int CLMSpi::quit()
{
    spi_private* p =(spi_private*)m_priv;
    p->quit_flag = 1;
    return 0;
}

int CLMSpi::isquit()
{
    return quit_flag;
}

//int CLMSpi::order(const char *symbol, int dir, double price, int num)
//{
//    //CTraderSpi* spi = static_cast<CTraderSpi*>(this);
//    CUstpFtdcInputOrderField ord;
//    memset(&ord, 0, sizeof(ord));

//    ///经纪公司编号
//    TUstpFtdcBrokerIDType	BrokerID;
//    ///交易所代码
//    TUstpFtdcExchangeIDType	ExchangeID;
//    ///系统报单编号 。置空
//    TUstpFtdcOrderSysIDType	OrderSysID;
//    ///投资者编号
//    TUstpFtdcInvestorIDType	InvestorID;
//    ///用户代码
//    TUstpFtdcUserIDType	UserID;
//    ///合约代码
//    TUstpFtdcInstrumentIDType	InstrumentID;
//    ///用户本地报单号 。用户自己维护，用来跟踪报单状态 integer,  递增，%012d
//    TUstpFtdcUserOrderLocalIDType	UserOrderLocalID;
//    ///报单类型
//    TUstpFtdcOrderPriceTypeType	OrderPriceType;
//    ///买卖方向
//    TUstpFtdcDirectionType	Direction;
//    ///开平标志
//    TUstpFtdcOffsetFlagType	OffsetFlag;
//    ///投机套保标志
//    TUstpFtdcHedgeFlagType	HedgeFlag;
//    ///价格
//    TUstpFtdcPriceType	LimitPrice;
//    ///数量
//    TUstpFtdcVolumeType	Volume;
//    ///有效期类型
//    TUstpFtdcTimeConditionType	TimeCondition;
//    ///GTD日期 yyyymmdd
//    TUstpFtdcDateType	GTDDate;
//    ///成交量类型
//    TUstpFtdcVolumeConditionType	VolumeCondition;
//    ///最小成交量
//    TUstpFtdcVolumeType	MinVolume;
//    ///止损价
//    TUstpFtdcPriceType	StopPrice;
//    ///强平原因
//    TUstpFtdcForceCloseReasonType	ForceCloseReason;
//    ///自动挂起标志
//    TUstpFtdcBoolType	IsAutoSuspend;
//    ///业务单元 。没有用
//    TUstpFtdcBusinessUnitType	BusinessUnit;
//    ///用户自定义域 。服务器原样返回
//    TUstpFtdcCustomType	UserCustom;
//    ///本地业务标识 。没有用
//    TUstpFtdcBusinessLocalIDType	BusinessLocalID;
//    ///业务发生日期 。交易日
//    TUstpFtdcDateType	ActionDay;



//#ifdef OPT_LOG_DEBUG

//	int64_t systime = 0;
//	get_system_time(&systime);
//	EalBson bson;
//	bson.AppendInt64("time", systime);

//	bson.AppendSymbol("BrokerID", ord.BrokerID);
//	bson.AppendSymbol("ExchangeID", ord.ExchangeID);
//	bson.AppendSymbol("OrderSysID", ord.OrderSysID);
//	bson.AppendSymbol("InvestorID", ord.InvestorID);
//	bson.AppendSymbol("UserID", ord.UserID);
//	bson.AppendSymbol("InstrumentID", ord.InstrumentID);
//	bson.AppendSymbol("UserOrderLocalID", ord.UserOrderLocalID);
//	bson.AppendFlag("OrderPriceType", ord.OrderPriceType);
//	bson.AppendFlag("Direction", ord.Direction);
//	bson.AppendFlag("OffsetFlag", ord.OffsetFlag);
//	bson.AppendFlag("HedgeFlag", ord.HedgeFlag);
//	bson.AppendDouble("LimitPrice",ord.LimitPrice);
//	bson.AppendInt64("Volume",ord.Volume);
//	bson.AppendFlag("TimeCondition", ord.TimeCondition);
//	bson.AppendSymbol("GTDDate", ord.GTDDate);
//	bson.AppendFlag("VolumeCondition", ord.VolumeCondition);
//	bson.AppendInt64("MinVolume",ord.MinVolume);
//	bson.AppendDouble("StopPrice",ord.StopPrice);
//	bson.AppendFlag("ForceCloseReason", ord.ForceCloseReason);
//	bson.AppendInt64("MinVolume",ord.MinVolume);
//	bson.AppendInt64("IsAutoSuspend",ord.IsAutoSuspend);
//	bson.AppendSymbol("BusinessUnit", ord.BusinessUnit);
//	bson.AppendSymbol("UserCustom", ord.UserCustom);
//	bson.AppendInt64("BusinessLocalID",ord.BusinessLocalID);
//	bson.AppendSymbol("ActionDay", ord.ActionDay);


//	const char *strJson = bson.GetJsonData();

//	logging("[future opt] send ( order ), content: %s", strJson);

//	bson.FreeJsonData();

//#endif

//	//logging_bson_order( (void *)&ord );

//    return 0;

//}


/** Implementation of the C api interface */

lmspi_t lmspi_create(const char* name, int poolsize) {
    CLMSpi * pt = new CLMSpi(name , poolsize);
    return (lmspi_t)pt;
}

void lmspi_delete(lmspi_t spi) {
    CLMSpi * pt = (CLMSpi *)spi;
    delete pt;
}

int lmspi_publish(lmspi_t spi, const char* symbol) {
    CLMSpi * pt = (CLMSpi *)spi;

    pt->publish(symbol);

    return 0;
}

int lmspi_subscribe(lmspi_t spi, const char* symbol) {
    CLMSpi * pt = (CLMSpi *)spi;

    pt->subscribe(symbol);

    return 0;
}

int lmspi_unpublish(lmspi_t spi, const char* symbol) {
    CLMSpi * pt = (CLMSpi *)spi;

    pt->unsubscribe(symbol);

    return 0;
}

int lmspi_unsubscribe(lmspi_t spi, const char* symbol) {
    CLMSpi * pt = (CLMSpi *)spi;

    pt->unpublish(symbol);

    return 0;
}

int lmspi_register_recv(lmspi_t spi, const char* symbol, symbol_callback *callback) {
    CLMSpi * pt = (CLMSpi *)spi;
    int ret;

    ret = pt->register_callback(*callback, symbol);
    return ret;
}

int lmspi_unregister_recv(lmspi_t spi, const char* symbol) {
    CLMSpi * pt = (CLMSpi *)spi;
    int ret;

    ret = pt->register_callback(NULL, symbol);
    return ret;
}


int lmspi_join(lmspi_t spi) {
    CLMSpi * pt = (CLMSpi *)spi;
    int ret;

    ret = pt->join();

    return ret;
}

void lmspi_quit(lmspi_t spi) {
    CLMSpi * pt = (CLMSpi *)spi;
    pt->quit();

}

void lmspi_logging(lmspi_t spi, const char* format, ...) {
    CLMSpi * pt = (CLMSpi *)spi;
    va_list argptr;
    char buf[512]={0};

    va_start(argptr, format);
    vsnprintf(buf, 511, format, argptr);
    va_end(argptr);

    pt->logging(buf);
}

void lmspi_send(lmspi_t spi, const char* symbol, const void* addr, int len) {
    CLMSpi * pt = (CLMSpi *)spi;

    pt->send(symbol, addr, len);

}

void lmspi_signal(lmspi_t spi, sig_t sigfunc) {
    CLMSpi * pt = (CLMSpi *)spi;
    pt->register_signal(sigfunc);
}
