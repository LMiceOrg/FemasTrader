#include <lmspi.h>  /* LMICED spi */

/* C lib */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#include <eal/lmice_trace.h>    /* EAL tracing */
#include <eal/lmice_bloomfilter.h> /* EAL Bloomfilter */
#include <eal/lmice_eal_hash.h>
#include <eal/lmice_eal_thread.h>
#include <eal/lmice_eal_spinlock.h>

/* system lib */
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

/* C++ lib */
#include <vector>
#include <string>

#include "fm2spi_ht.h"
#include "fm3in1_ht.h"


/** Function declaration */

/* print usage of app */
forceinline void print_usage(void);

/* Daemon */
forceinline int init_daemon(int silent);

/* guava md */
forceinline void guava_md(int mc_port, const char* mc_group, const char* mc_bindip, fm3_args_t* args);

/* publish data */
forceinline void netmd_pub_data(const char* sym, const void* addr, int len, fm3_args_t* args);
static void guava_md_stop(int sig);
static void guava_sf_flatten(int sig);

/* md func */
forceinline void md_func(Dummy_ChinaL1Msg& msg_dt, const void* addr, int size, fm3_args_t* args, int instrument_no);
/* init forecaster*/
forceinline void init_forecaster(Forecaster** fc);

/** Global vars */

/* flatten flag */

CFemas2TraderSpi* g_spi;
//static fm3_args_t* g_args = 0;
static volatile int* g_guava_quit_flag = 0;
volatile int* g_flatten_flag = 0;
volatile int* g_orderaction_flag = 0;

//volatile int& g_flatten_flag = g_args->cur_status.flatten_flag;
//volatile int& g_orderaction_flag = g_args->orderaction_flag;

static void guava_md_stop(int sig) {

    (void)sig;
    *g_guava_quit_flag = 1;
}

static void guava_sf_flatten(int sig) {
    if( sig == SIGUSR1 )
    {
        *g_flatten_flag = 1;
    }
}

int main(int argc, char* argv[]) {

    uid_t uid = (uid_t)-1;
    int ret;
    CUstpFtdcTraderApi *pt = 0;
    int i;
    fm3_args_t args;
    //int mc_sock = 0;
    //int mc_port = 30100;/* 5001*/
    //char mc_group[32]="233.54.1.100"; /*238.0.1.2*/
    //char mc_bindip[32]="192.168.208.16";/*10.10.21.191*/



#if 0
    char devname[64] = "p6p1";
    char filter[128] = "udp and port 30100";
#endif

    g_flatten_flag = &args.cur_status.flatten_flag;
    g_guava_quit_flag = &args.guava_quit_flag;
    g_orderaction_flag = &args.orderaction_flag;

    CUstpFtdcInputOrderField &g_order=args.order;
    CUstpFtdcOrderActionField &g_order_action =args.order_action;
    CUR_STATUS &g_cur_status = args.cur_status;
    STRATEGY_CONF &st_conf = args.st_conf;

    fm3_config_t& cfg = args.config;
    int& silent = cfg.silent;
    int& test = cfg.test;
    int &position = cfg.position;
    int &bytes = cfg.bytes;
    int &g_account_id = cfg.account_id;
    struct itimerval& g_tick = cfg.flatten_tick;
    /** Trader */
    typedef char(c64)[64];
    typedef char(c32)[32];
    c64& front_address = cfg.front_address;
    c64& user=cfg.user;
    c64& password = cfg.password;
    c64& broker = cfg.broker;
    c64& investor = cfg.investor;
    c32& exchange_id= cfg.exchange_id;
    c32& md_name = cfg.md_name;
    /** MD */
    int& mc_port = cfg.mc_port;
    c32& mc_group = cfg.mc_group;
    c32& mc_bindip = cfg.mc_bindip;

    /* Init OrderInsert Flags */
    g_cur_status.done_flag = 1;

    /** Init args, forecaster */
    init_forecaster(&args.forecaster);


    //register signal
    signal(SIGUSR1, guava_sf_flatten); //SIGUSER1 = 10

    //memset( &g_cur_status, 0, sizeof(CUR_STATUS));
    //to be add,add trade insturment message and update to st
    g_cur_status.m_md.fee_rate = 0.000101 + 0.0000006;
    g_cur_status.m_md.m_up_price = 2847;
    g_cur_status.m_md.m_down_price = 2524;
    g_cur_status.m_md.m_last_price = 0;
    g_cur_status.m_md.m_multiple = 10;

    //memset(&st_conf, 0, sizeof(STRATEGY_CONF));

    //netmd_pd_lock = 0;
    //keypos = 0;
    memset(md_name, 0, sizeof(md_name));
    strcpy(md_name, "[FM3In1 HT]");
    /** Process command line */
    for(i=0; i< argc; ++i) {
        const char* cmd = argv[i];
        if(     strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
            printf("args.order:0x%llx\nargs.tags:0x%llx\nsize:0x%lx\n",
                   &((fm3_args_t*)0)->order,
                   ((fm3_args_t*)0)->tags,
                   sizeof(args));
            return 0;
        } else if(strcmp(cmd, "-t") == 0 ||
                  strcmp(cmd, "--test") == 0) {
            lmice_critical_print("Unit test\n");
            test = 1;

        } else if(strcmp(cmd, "-n") == 0 ||
                  strcmp(cmd, "--name") == 0) {
            if(i+1<argc) {
                cmd=argv[i+1];
                memset(md_name, 0, sizeof(md_name));
                strncat(md_name, cmd, sizeof(md_name)-1);
            } else {
                lmice_error_print("Command(%s) require module name\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-d") == 0 ||
                  strcmp(cmd, "--device") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                if(strlen(cmd)>63) {
                    lmice_error_print("Adapter device name is too long(>63)\n");
                    return 1;
                }
#if 0
                memset(devname, 0, sizeof(devname));
                strncat(devname, cmd, sizeof(devname)-1);
#endif
                lmice_warning_print("Do NOT support pcap[device=%s] \n", cmd);
            } else {
                lmice_error_print("Command(%s) require device name\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-s") == 0 ||
                  strcmp(cmd, "--silent") == 0) {
            silent = 1;
        } else if(strcmp(cmd, "-u") == 0 ||
                  strcmp(cmd, "--uid") == 0) {
            if(i+1<argc){
                cmd = argv[i+1];
                uid = static_cast<uint32_t>(atoi(cmd));
                ret = seteuid(uid);
                if(ret != 0) {
                    ret = errno;
                    lmice_error_print("Change to user(%d) failed as %d.\n", uid, ret);
                    return ret;
                }
            } else {
                lmice_error_print("Command(%s) require device name\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-f") == 0 ||
                  strcmp(cmd, "--filter") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                ret = strlen(cmd);
                if(ret > 128) {
                    lmice_error_print("filter string is too long(>127)\n");
                    return ret;
                }
#if 0
                memset(filter, 0, sizeof(filter));
                strncpy(filter, cmd, ret);
#endif
                lmice_warning_print("Do NOT support pcap[filter=%s] \n", cmd);
            } else {
                lmice_error_print("Command(%s) require filter string\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-p") == 0 ||
                  strcmp(cmd, "--position") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                position = atoi(cmd);
            } else {
                lmice_error_print("Command(%s) require position number\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-b") == 0 ||
                  strcmp(cmd, "--bytes") == 0) {
            if(i+1<argc) {
                cmd = argv[i+1];
                bytes = atoi(cmd);
            } else {
                lmice_error_print("Command(%s) require package size number\n", cmd);
                return 1;
            }
            ++i;
        } else if(strcmp(cmd, "-m") == 0 ||
                  strcmp(cmd, "--multicast") == 0) {
            if(i+3<argc) {
                cmd = argv[i+1];
                memset(mc_group, 0, sizeof(mc_group));
                strncpy(mc_group, cmd, 32);
                cmd=argv[i+2];
                memset(mc_bindip, 0, sizeof(mc_bindip));
                strncpy(mc_bindip, cmd, 32);
                cmd=argv[i+3];
                mc_port = atoi(cmd);
            } else {
                lmice_error_print("Command(%s) require mc_group, bind_ip port param(3)\n", cmd);
                return 1;
            }
            i+=3;
        } else if(strcmp(cmd, "-c") == 0 ||
                  strcmp(cmd, "--cpuset") == 0) {
            if(i+1 < argc) {
                int cpuset[8];
                int setcount = 0;
                cmd = argv[i+1];

                do {
                    if(setcount >=8)
                        break;
                    cpuset[setcount] = atoi(cmd);
                    ++setcount;
                    while(*cmd != 0) {
                        if(*cmd != ',') {
                            ++cmd;
                        } else {
                            ++cmd;
                            break;
                        }
                    }
                } while(*cmd != 0);

                ret = lmspi_cpuset(NULL, cpuset, setcount);
                lmice_critical_print("set CPUset %d return %d\n", setcount, ret);

            } else {
                lmice_error_print("Command(%s) require cpuset param, separated by comma(,)\n", cmd);
            }
            ++i;
        }else if(strcmp(cmd, "-sv") == 0 ||
                 strcmp(cmd, "--svalue") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
                st_conf.m_close_value = atof(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require yesterday close value(double)\n", cmd);
                return 1;
            }
        }
        else if(strcmp(cmd, "-sp") == 0 ||
                strcmp(cmd, "--spostition") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
                st_conf.m_max_pos = atoi(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require max position can hold(uint)\n", cmd);
                return 1;
            }
        }
        else if(strcmp(cmd, "-sl") == 0 ||
                strcmp(cmd, "--sloss") == 0)
        {
            if(i+1<argc) {
                cmd = argv[++i];
                st_conf.m_max_loss = atof(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require max loss can make(double)\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tf") == 0 ||
                  strcmp(cmd, "--tfront") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(front_address, 0, sizeof(front_address));
                strncpy(front_address, cmd, sizeof(front_address)-1);
            } else {
                lmice_error_print("Trader Command(%s) require front address string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tu") == 0 ||
                  strcmp(cmd, "--tuser") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(user, 0, sizeof(user));
                strncpy(user, cmd, sizeof(user)-1);
                //用户ID
                strncpy(investor, user+strlen(broker), 8);
                g_account_id = (atoi(user + FM_ACCOUNT_GAP) - FM_ACCOUNT_DIFF) % 10;
            } else {
                lmice_error_print("Trader Command(%s) require user id string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tp") == 0 ||
                  strcmp(cmd, "--tpassword") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(password, 0, sizeof(password));
                strncpy(password, cmd, sizeof(password)-1);
            } else {
                lmice_error_print("Trader Command(%s) require password string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-tb") == 0 ||
                  strcmp(cmd, "--tbroker") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(broker, 0, sizeof(broker));
                strncpy(broker, cmd, sizeof(broker)-1);
                //投资人ID
                strncpy(investor, user+strlen(broker), 8);
                //investor = user + strlen(broker);
            } else {
                lmice_error_print("Trader Command(%s) require broker string\n", cmd);
                return 1;
            }
        } else if(strcmp(cmd, "-te") == 0 ||
                  strcmp(cmd, "--texchange") == 0) {
            if(i+1 < argc) {
                cmd = argv[i+1];
                memset(exchange_id, 0, sizeof(exchange_id));
                strncpy(exchange_id, cmd, sizeof(exchange_id)-1);
            } else {
                lmice_error_print("Trader Command(%s) require exchange id string\n", cmd);
                return 1;
            }
        }
    } /* end-for: argc*/


    lmice_critical_print("FM3In1 HT[%x] -- 3in1 HaiTong app --\n", pthread_self());
    pthread_setname_np(pthread_self(), md_name);

    /** Silence mode */
    init_daemon(silent);

    /** Femas Trader */
    /// 注册SPI

    // CUstpFtdcTraderApi
    pt = CUstpFtdcTraderApi::CreateFtdcTraderApi("");
    //SPI
    g_spi = new CFemas2TraderSpi(pt, md_name, args);


    // 设置交易信息
    g_spi->user_id(user);
    g_spi->password(password);
    g_spi->broker_id(broker);
    g_spi->investor_id(investor);
    g_spi->front_address(front_address);
    g_spi->model_name(md_name);
    g_spi->exchange_id(exchange_id);

    g_spi->init_trader();

    // 注册
    pt->RegisterSpi(g_spi);

    // 订阅
    //        TERT_RESTART:从本交易日开始
    //        TERT_RESUME:从上次收到的续传
    //        TERT_QUICK:只传送登录后私有流的内容
    pt->SubscribePrivateTopic( USTP_TERT_QUICK );
    pt->SubscribePublicTopic ( USTP_TERT_QUICK );

    //设置心跳超时时间
    pt->SetHeartbeatTimeout(5);

    //注册前置机网络地址
    pt->RegisterFront(front_address);


    pt->Init();


    /** Order */
    memset( &g_order, 0, sizeof(CUstpFtdcInputOrderField));
    g_order.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
    g_order.HedgeFlag = USTP_FTDC_CHF_Speculation;
    g_order.TimeCondition = USTP_FTDC_TC_GFD;
    g_order.VolumeCondition = USTP_FTDC_VC_AV;
    g_order.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
    strcpy(g_order.BrokerID, g_spi->broker_id());
    strcpy(g_order.ExchangeID, g_spi->exchange_id());
    strcpy(g_order.InvestorID, g_spi->investor_id());
    strcpy(g_order.UserID, g_spi->user_id());

    /** Order Action */
    memset(&g_order_action, 0, sizeof(CUstpFtdcOrderActionField));
    strcpy(g_order_action.BrokerID, g_spi->broker_id());
    strcpy(g_order_action.ExchangeID, g_spi->exchange_id());
    strcpy(g_order_action.InvestorID, g_spi->investor_id());
    strcpy(g_order_action.UserID, g_spi->user_id());
    g_order_action.ActionFlag = USTP_FTDC_AF_Delete;

    /** Strategy */
    {
        std::string inst = args.forecaster->get_trading_instrument();

        memcpy(&args.tags[0], inst.c_str(), inst.size()>8?8:inst.size());
        args.msgs[0].m_inst = inst;

        strcpy(g_cur_status.m_ins_name, args.forecaster->get_trading_instrument().c_str());
        strcpy(g_order.InstrumentID, inst.c_str());
        strcpy(cfg.trading_instrument, inst.c_str());

        std::vector<std::string> array_ref_ins = args.forecaster->get_subscriptions();
        lmice_info_print("trading symbol:%s\n", inst.c_str());
        for(size_t i=0; i<array_ref_ins.size() && i<7; ++i) {
            lmice_info_print("ref symbol:%s\n", array_ref_ins[i].c_str());
            args.msgs[i+1].m_inst = array_ref_ins[i];
            memcpy(&args.tags[i+1], array_ref_ins[i].c_str(), array_ref_ins[i].size()>8?8:array_ref_ins[i].size());

        }
    }

    /** timer init */
    memset(&g_tick, 0, sizeof(struct itimerval));
    g_tick.it_value.tv_sec = 5;
    g_tick.it_value.tv_usec = 0;

    /** deadtime 5sec orderaction signal */
    g_spi->register_signal(guava_md_stop);

    /** Unit test */
    if(test) {
        lmice_critical_print("No unit test\n");
    } else {
        /** Run in main thread */
        //netmd_pcap_thread(devname, filter);
        guava_md(mc_port, mc_group, mc_bindip, &args);
        /** flatten all */
        g_spi->flatten_all(0, 0, 0);
    }



    /** Exit and maintain resource */
    lmice_warning_print("Exit %s\n", md_name);

    //netmd_bf_delete();

    // 释放useapi实例
    pt->Release();
    //释放SPI实例
    delete g_spi;

    return 0;
}



forceinline void print_usage(void) {
    printf("NetMD -- a md app --\n\n"
           "\t-h, --help\t\tshow this message\n"
           "\t-n, --name\t\tset module name\n"
           "\t-d, --device\t\tset adapter device name\n"
           "\t-u, --uid\t\tset user id when running\n"
           "\t-s, --silent\t\trun in silent mode[backend]\n"
           "\t-f, --filter\t\tfilter to run pcap\n"
           "\t-p, --position\t\tposition to catch symbol\n"
           "\t-b, --bytes\t\tpackage size limitation\n"
           "\t-m, --multicast\t\tmulticast group, bind ip, port\n"
           "\t-c, --cpuset\t\tSet cpuset for this app(, separated)\n"
           "\t-t, --test\t\tUnit test\n"

           "strategyht: -- strategyht app --\n\n"
           "\t-sv, --svalue\t\tyesterday value of corresponding account\n"
           "\t-sp, --sposition\t\tmax position can hold\n"
           "\t-sl, --sloss\t\tmax loss can make\n"

           "fm2trader: -- Femas2.0 Trader app --\n\n"
           "\t-tf, --tfront\t\tset front address\n"
           "\t-tu, --tuser\t\tset user id \n"
           "\t-tp, --tpassword\t\tset password\n"
           "\t-tb, --tbroker\t\tset borker id\n"
           "\t-te, --texchange\t\tset exchange id\n"
           "\n"
           );
    printf("%llx, %llx\n", &((fm3_args_t*)(0))->data.AskPrice1,
           &((fm3_args_t*)(0))->data.BidPrice1);
}

forceinline int init_daemon(int silent) {
    if(!silent)
        return 0;

    if( daemon(0, 1) != 0) {
        return -2;
    }
    return 0;
}

forceinline void guava_md(int mc_port, const char* mc_group, const char* mc_bindip, fm3_args_t* args) {
    char* line = args->line;
    int options;
    int m_sock = socket(PF_INET, SOCK_DGRAM, 0);

    int&position = args->config.position;
    int& g_order_action_count =args->order_action_count;
    volatile int& g_guava_quit_flag = args->guava_quit_flag;
    int& n_rcved = args->n_rcved;

    if(-1 == m_sock)
    {
        return;
    }

    //socket可以重新使用一个本地地址
    options=1;
    if(setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&options, sizeof(options)) != 0)
    {
        return;
    }

    options =  fcntl(m_sock, F_GETFL) | O_NONBLOCK;
    int i_ret = fcntl(m_sock, F_SETFL, options);
    if(i_ret < 0)
    {
        return;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(mc_port);	//multicast port
    if (bind(m_sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        return;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(mc_group);	//multicast group ip
    mreq.imr_interface.s_addr = inet_addr(mc_bindip);

    if (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
    {
        return;
    }

    /*receive_buf_size*/
    options = 65536;
    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&options, sizeof(options)) != 0)
    {
        return;
    }

    struct sockaddr_in muticast_addr;
    socklen_t len = sizeof(sockaddr_in);
    for(;;) {

        n_rcved = recvfrom(m_sock, line, FM3_RECV_BUFF_SIZE, 0, (struct sockaddr*)&muticast_addr, &len);

        //检测线程退出
        if (g_guava_quit_flag == 1)
        {
            //此时已关闭完所有的客户
            return;
        }

        if ( n_rcved > 0)
        {
            netmd_pub_data(line+position, line, n_rcved, args);
            if( g_order_action_count >=496 )
            {
                lmice_critical_print("\nreach the max order action times, exit!\n");
                return;
            }

        }
        else
        {
            continue;
        }


    }
}

forceinline void netmd_pub_data(const char* sym, const void* addr, int len, fm3_args_t* args) {
    int i;
    const int64_t *symid = reinterpret_cast<const int64_t*>(sym);
    const SHFEQuotDataTag *md_data = &(args->data);
    int64_t& g_begin_time = args->begin_time;

    if(g_spi->status() != FMTRADER_LOGIN) {
        return;
    }

    get_system_time(&g_begin_time);

    for(i=0; i< FM3_INSTRUMENT_SIZE; ++i) {
        if (args->tags[i] == *symid) {
            Dummy_ChinaL1Msg &msg_dt = args->msgs[i];
            //msg_dt.m_inst = md_data->InstrumentID;
            msg_dt.m_time_micro = g_begin_time/10;
            msg_dt.m_bid = md_data->BidPrice1;
            msg_dt.m_offer = md_data->AskPrice1;
            msg_dt.m_bid_quantity = md_data->BidVolume1;
            msg_dt.m_offer_quantity = md_data->AskVolume1;
            msg_dt.m_volume = md_data->Volume ;
            msg_dt.m_notional = md_data->Turnover;
            msg_dt.m_limit_up = 0;
            msg_dt.m_limit_down = 0;
            md_func(msg_dt, addr, len, args, i);
            break;
        }
    }


}


forceinline void do_order_insert(fm3_args_t*args, int volume) {
    int64_t&g_begin_time = args->begin_time;
    int64_t& middle_time = args->proc_time;
    int64_t &g_end_time = args->end_time;
    CUstpFtdcInputOrderField&g_order=args->order;

    get_system_time(&middle_time);
    g_spi->order_insert(0, &g_order, sizeof(CUstpFtdcInputOrderField));
    get_system_time(&g_end_time);
    printf(//"ttal time:%lld\t"
           "inst time:%lld\t"
           "proc time:%lld\t"
           "operation:%d\t"
           "cur price:%lf\t"
           "leftvlume:%d\t"
           "ordr size:%d\n",
          // g_end_time - ( g_pkg_time.tv_sec*10000000L+g_pkg_time.tv_usec*10L),
           g_end_time - middle_time,
           g_end_time - g_begin_time,
           g_order.OffsetFlag,
           g_order.LimitPrice,
           volume,
           g_order.Volume);
}

/// cannel timer
forceinline void uninit_time()
{
    struct itimerval value;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 0;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL, &value, NULL);
}

forceinline int dbl_notequal(double d1, double d2) {
    double e = 10e-5;
    if(d2 < d1-e ||
            d2 > d1+e)
        return 1;
    return 0;
}


forceinline void md_func(Dummy_ChinaL1Msg& msg_dt, const void* addr, int size, fm3_args_t* args, int instrument_no) {
    ChinaL1Msg& msg_data = *reinterpret_cast<ChinaL1Msg*>(&msg_dt);
    Forecaster* fc = args->forecaster;
    CUR_STATUS* g_cur_st = &args->cur_status;
    STRATEGY_CONF*g_conf = &args->st_conf;
    const SHFEQuotDataTag *md_data = reinterpret_cast<const SHFEQuotDataTag *>(addr);

    int &g_done_flag = g_cur_st->done_flag;
    int &g_flatten_flag = g_cur_st->flatten_flag;
    int &g_active_buy_orders = g_cur_st->active_buy_orders;
    int &g_active_sell_orders = g_cur_st->active_sell_orders;
    CUstpFtdcInputOrderField&    g_order = args->order;


    const uint64_t d_max = 0x7fefffffffffffff;

    if(size == sizeof(SHFEQuotDataTag) &&
            *(const uint64_t*)(&md_data->AskPrice1) != d_max &&
            *(const uint64_t*)(&md_data->BidPrice1) != d_max &&
            *(const uint64_t*)(&md_data->LastPrice) != d_max )
    {

        fc->update(msg_data);
        double signal = fc->get_forecast();
        double forecast = 0;

        if( 0 == instrument_no )
        {
            if( md_data->LastPrice > 0 &&
                    dbl_notequal(md_data->LastPrice, g_cur_st->m_md.m_last_price) )
            {
                g_cur_st->m_md.m_last_price =  md_data->LastPrice;
                //fix me -  modify : buy sell count, value calculate methods, fee, multiplier
                int left_pos = g_cur_st->m_pos.m_buy_pos - g_cur_st->m_pos.m_sell_pos;
                double fee =  abs(left_pos) * g_cur_st->m_md.m_multiple * g_cur_st->m_md.m_last_price * g_cur_st->m_md.fee_rate;
                double pl = g_cur_st->m_acc.m_left_cash + left_pos * g_cur_st->m_md.m_multiple * g_cur_st->m_md.m_last_price - fee;

                if( -pl >= g_conf->m_max_loss )
                {
                    g_spi->quit();
                    args->guava_quit_flag = 1;
                    sleep(1);

                    lmice_critical_print("touch max loss,stop and exit\n");
                }

            }

            if( g_cur_st->m_md.m_last_price >= g_cur_st->m_md.m_up_price ||
                    g_cur_st->m_md.m_last_price <= g_cur_st->m_md.m_down_price )
            {
                printf("touch the limit, return\n"
                        "last price:%lf\n"
                        "up price:%lf\n"
                        "down price:%lf\n",
                       g_cur_st->m_md.m_last_price,
                       g_cur_st->m_md.m_up_price,
                       g_cur_st->m_md.m_down_price);
                return;
            }

            double mid = 0.5 * ( md_data->AskPrice1 + md_data->BidPrice1 );
            //				signal = signal/1.21;
            forecast = mid * ( 1 + signal );

            //send order insert command
            int order_size = 0;
            int left_pos_size = 0;

            if( g_done_flag == 1 )
            {
                if( g_flatten_flag == 0)
                {
                    if( forecast > md_data->AskPrice1 )
                    {
                        if( g_active_buy_orders > 0 )
                        {
                            //printf("/t/t===buy=== g_active_buy_orders is %d, return\n", g_active_buy_orders);
                            return;
                        }
                        if( g_active_sell_orders > 0 )
                        {
                            //printf("/t/t===buy=== g_active_sell_orders is %d, order action and return\n ", g_active_sell_orders);
                            uninit_time();
                            g_spi->order_action();
                            g_active_sell_orders = 0;
                        }
                        left_pos_size = g_conf->m_max_pos - ( g_cur_st->m_pos.m_buy_pos - g_cur_st->m_pos.m_sell_pos );
                        //printf("=== left_pos_size: %d\n ===\n", left_pos_size);
                        if( left_pos_size <= 0 )
                        {
                            lmice_warning_print("[ASK]left pos size <0\n");
                            return;
                        }
                        order_size = left_pos_size ;//< md_data->AskBidInst.AskVolume1 ? left_pos_size : md_data->AskBidInst.AskVolume1;
                        g_order.Volume = order_size;
                        g_order.LimitPrice = md_data->AskPrice1;
                        //printf("=== order_size: %d\n ===\n", order_size);
                        if( g_cur_st->m_pos.m_sell_pos >= order_size )
                        {
                            g_order.Direction = USTP_FTDC_D_Buy;
                            g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
                        }
                        else
                        {
                            g_order.Direction = USTP_FTDC_D_Buy;
                            g_order.OffsetFlag = USTP_FTDC_OF_Open;
                        }
                        //send order insert
                        do_order_insert(args, md_data->AskVolume1);
                        g_done_flag = 0;
                    }

                    if( forecast < md_data->BidPrice1 )
                    {
                        if( g_active_sell_orders > 0 )
                        {
                            //printf("/t/t===sell=== g_active_sell_orders is %d, return\n", g_active_sell_orders);
                            return;
                        }
                        if( g_active_buy_orders > 0 )
                        {
                            //printf("/t/t===sell=== g_active_buy_orders is %d, order action and return\n ", g_active_buy_orders);
                            uninit_time();
                            g_spi->order_action();
                            g_active_buy_orders = 0;
                        }


                        left_pos_size = g_conf->m_max_pos - ( g_cur_st->m_pos.m_sell_pos - g_cur_st->m_pos.m_buy_pos );
                        if( left_pos_size <= 0 )
                        {
                            lmice_warning_print("[BID]left pos size <0\n");
                            return;
                        }
                        order_size = left_pos_size; /*< md_data->AskBidInst.BidVolume1 ?
                                            left_pos_size : md_data->AskBidInst.BidVolume1;*/
                        g_order.Volume = order_size;
                        g_order.LimitPrice = md_data->BidPrice1;

                        if( g_cur_st->m_pos.m_buy_pos >= order_size )
                        {
                            g_order.Direction = USTP_FTDC_D_Sell;
                            g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
                        }
                        else
                        {
                            g_order.Direction = USTP_FTDC_D_Sell;
                            g_order.OffsetFlag = USTP_FTDC_OF_Open;
                        }
                        //send order insert
                        do_order_insert(args, md_data->BidVolume1);
                        g_done_flag = 0;
                    }
                }
                else
                {
                    if( forecast > md_data->AskPrice1 )
                    {
                        if( g_active_buy_orders > 0 || g_active_sell_orders > 0)
                        {
                            uninit_time();
                            g_spi->order_action();
                        }
                        order_size = g_cur_st->m_pos.m_sell_pos;
                        if( order_size <= 0 )
                        {
                            lmice_warning_print("[ASK]left pos size <0\n");
                            return;
                        }
                        g_order.Volume = order_size;
                        g_order.LimitPrice = md_data->AskPrice1;
                        g_order.Direction = USTP_FTDC_D_Buy;
                        g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
                        //send order insert
                        do_order_insert(args, md_data->AskVolume1);
                        g_done_flag = 0;
                    }

                    if( forecast < md_data->BidPrice1 )
                    {
                        if( g_active_buy_orders > 0 || g_active_sell_orders > 0)
                        {
                            uninit_time();
                            g_spi->order_action();
                        }

                        order_size = g_cur_st->m_pos.m_buy_pos;
                        if( order_size <= 0 )
                        {
                            lmice_warning_print("[BID]left pos size <0\n");
                            return;
                        }
                        g_order.Volume = order_size;
                        g_order.LimitPrice = md_data->BidPrice1;
                        g_order.Direction = USTP_FTDC_D_Sell;
                        g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;

                        //send order insert
                        do_order_insert(args, md_data->BidVolume1);
                        g_done_flag = 0;
                    }

                }
            }

        }



    }

}



forceinline void init_forecaster(Forecaster** fcpp) {
    std::string type = "rb_0";
    int64_t micro_time = 0;
    struct tm date;
    time_t t;

    get_system_time(&micro_time);
    t = micro_time/10000000LL;
    localtime_r(&t, &date);

    *fcpp = ForecasterFactory::createForecaster( type, date );


}
