#include <lmspi.h>  /* LMICED spi */
#include <pcap.h>   /* libpcap  */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <eal/lmice_trace.h>    /* EAL tracing */
#include <eal/lmice_bloomfilter.h> /* EAL Bloomfilter */
#include <eal/lmice_eal_hash.h>
#include <eal/lmice_eal_thread.h>
#include <eal/lmice_eal_spinlock.h>

#include "fm3in1.h"
#include "guavaproto.h"
#include "strategy_ins.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <vector>
#include <string>

pcap_t* pcapHandle = NULL;

lm_bloomfilter_t* bflter = NULL;

/* strategy instance */
strategy_ins *g_ins = NULL;
char trading_instrument[32];
std::vector<std::string> array_ref_ins;

/* trader */

CFemas2TraderSpi* g_spi;

CUR_STATUS g_cur_status;
CUR_STATUS_P g_cur_st=&g_cur_status;
STRATEGY_CONF st_conf;
STRATEGY_CONF_P g_conf= &st_conf;
CUstpFtdcInputOrderField g_order;
int64_t g_begin_time = 0;
int64_t g_end_time = 0;
struct timeval g_pkg_time;

/* netmd package callback */
static void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet);

/* netmd worker thread */
static int netmd_pcap_thread(const char *devname, const char *packet_filter);

/* stop netmd worker */
static void netmd_pcap_stop(int sig);

/* publish data */
forceinline void netmd_pub_data(const char* symbol, const void* addr, int len);

/* create bloom filter for net md */
static void netmd_bf_create(void);

/* delete bloom filter */
static void netmd_bf_delete(void);

/* print usage of app */
static void print_usage(void);

/* Daemon */
static int init_daemon(int silent);

/* Unit test */
static void unit_test(void);

static int position = 11;
static int bytes = sizeof(struct guava_udp_normal)+sizeof(struct guava_udp_head);
static char md_name[32];
/*
#define MAX_KEY_LENGTH 512
static uint64_t keylist[MAX_KEY_LENGTH];
static volatile int keypos;
static volatile int64_t netmd_pd_lock = 0;
*/
int main(int argc, char* argv[]) {
    uid_t uid = -1;
    int ret;
    int i;
    int silent = 0;
    int test = 0;

    int mc_sock = 0;
    int mc_port = 30100;/* 5001*/
    char mc_group[32]="233.54.1.100"; /*238.0.1.2*/
    char mc_bindip[32]="192.168.208.16";/*10.10.21.191*/

    char devname[64] = "p6p1";
    char filter[128] = "udp and port 30100";

    /** Trader */
    char front_address[64] = "10.0.10.0";
    char user[64] = "";
    char password[64] = "";
    char broker[64] = "";
    char *investor = user;
    char exchange_id[32]= "SHIF";


    (void)argc;
    (void)argv;

    memset( &g_cur_status, 0, sizeof(CUR_STATUS));
    //to be add,add trade insturment message and update to st
    g_cur_status.m_md.fee_rate = 0.000101 + 0.0000006;
    g_cur_status.m_md.m_up_price = 2787;
    g_cur_status.m_md.m_down_price = 2472;
    g_cur_status.m_md.m_last_price = 0;
    g_cur_status.m_md.m_multiple = 10;

    memset(&st_conf, 0, sizeof(STRATEGY_CONF));

    //netmd_pd_lock = 0;
    //keypos = 0;
    memset(md_name, 0, sizeof(md_name));
    strcpy(md_name, "[fm3in1]");

    /** Process command line */
    for(i=0; i< argc; ++i) {
        char* cmd = argv[i];
        if(     strcmp(cmd, "-h") == 0 ||
                strcmp(cmd, "--help") == 0) {
            print_usage();
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
                memset(devname, 0, sizeof(devname));
                strncat(devname, cmd, sizeof(devname)-1);
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
                uid = atoi(cmd);
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
                memset(filter, 0, sizeof(filter));
                ret = strlen(cmd);
                if(ret > 128) {
                    lmice_error_print("filter string is too long(>127)\n");
                    return ret;
                }
                strncpy(filter, cmd, ret);
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
                //投资人ID＝ 用户ID［去除前面的交易商ID］
                investor = user + strlen(broker);
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
                //投资人ID＝ 用户ID［去除前面的交易商ID］
                investor = user + strlen(broker);
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



    lmice_critical_print("FM3In1[%x] -- a md app --\n", pthread_self());
    pthread_setname_np(pthread_self(), "NetMD");

    /** Silence mode */
    init_daemon(silent);

    /** Join MCast group */
    {
        struct ip_mreq mreq;
        struct sockaddr_in local_addr;
        int flag=1;
        mc_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        setsockopt(mc_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(mc_port);	//multicast port
        bind(mc_sock, (struct sockaddr*)&local_addr, sizeof(local_addr));


        mreq.imr_multiaddr.s_addr = inet_addr(mc_group);	//multicast group ip
        mreq.imr_interface.s_addr = inet_addr(mc_bindip);

        setsockopt(mc_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    }



    /** Femas Trader */
    /// 注册SPI

    // 产生一个 CUstpFtdcTraderApi 实例
    CUstpFtdcTraderApi *pt = CUstpFtdcTraderApi::CreateFtdcTraderApi("");
    //产生一个事件处理的实例
    g_spi = new CFemas2TraderSpi(pt, md_name);


    // 设置交易信息
    g_spi->user_id(user);
    g_spi->password(password);
    g_spi->broker_id(broker);
    g_spi->investor_id(investor);
    g_spi->front_address(front_address);
    g_spi->model_name(md_name);
    g_spi->exchange_id(exchange_id);

    g_spi->init_trader();

    // 注册一事件处理的实例
    pt->RegisterSpi(g_spi);

    // 订阅私有流
    //        TERT_RESTART:从本交易日开始重传
    //        TERT_RESUME:从上次收到的续传
    //        TERT_QUICK:只传送登录后私有流的内容
    pt->SubscribePrivateTopic( USTP_TERT_QUICK );
    pt->SubscribePublicTopic ( USTP_TERT_QUICK );

    //设置心跳超时时间
    pt->SetHeartbeatTimeout(5);

    //注册前置机网络地址
    pt->RegisterFront(front_address);

    //初始化
    //if(!test)
    {
        pt->Init();
    }

    /** Order */
    memset( &g_order, 0, sizeof(CUstpFtdcInputOrderField));
    g_order.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
    g_order.HedgeFlag = USTP_FTDC_CHF_Speculation;
    g_order.TimeCondition = USTP_FTDC_TC_IOC;
    g_order.VolumeCondition = USTP_FTDC_VC_AV;
    g_order.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
    strcpy(g_order.BrokerID, g_spi->broker_id());
    strcpy(g_order.ExchangeID, g_spi->exchange_id());
    strcpy(g_order.InvestorID, g_spi->investor_id());
    strcpy(g_order.UserID, g_spi->user_id());

    /** Strategy */
    g_ins = new strategy_ins(&st_conf, g_spi);
    memcpy(trading_instrument,
           g_ins->get_forecaster()->get_trading_instrument().c_str(),
           g_ins->get_forecaster()->get_trading_instrument().size());
    strcpy(g_cur_st->m_ins_name,g_ins->get_forecaster()->get_trading_instrument().c_str());
    strcpy(g_order.InstrumentID, trading_instrument);

    array_ref_ins = g_ins->get_forecaster()->get_subscriptions();
    lmice_info_print("trading symbol:%s\n", trading_instrument);
    for(i=0; i<array_ref_ins.size(); ++i) {
        lmice_info_print("ref symbol:%s\n", array_ref_ins[i].c_str());
    }


    /** Create LMiced spi */
//    g_spi = lmspi_create(md_name, -1);
//    lmice_info_print("%s startting in adapter[%s]. filter[%s]..\n", md_name, devname, filter);
//    if(uid != -1) {
//        ret = seteuid(getuid());
//        if(ret != 0) {
//            ret = errno;
//            lmice_error_print("Change to user(%d) failed as %d.\n", uid, ret);
//            return ret;
//        }
//    }

    /** Create bloom filter */
    //netmd_bf_create();

    /** Register signal handler */
    g_spi->register_signal(netmd_pcap_stop);

    /** Unit test */
    if(test) {
        unit_test();
    } else {
        /** Run in main thread */
        netmd_pcap_thread(devname, filter);
        /** flatten all */
        g_spi->flatten_all(0, 0, 0);
    }
    /*
     * for(ret=0; ret<10; ++ret) {
        char buff[32] = {0};
        sprintf(buff, "data is %d\n", ret);
        netmd_pub_data(spi, "rb1610", buff, 32);
        usleep(500000);
    }
    */


    /** Exit and maintain resource */
    lmice_warning_print("Exit %s\n", md_name);

    //netmd_bf_delete();

    // 释放useapi实例
    pt->Release();
    //释放SPI实例
    delete g_spi;

    return 0;
}


int init_daemon(int silent) {
    if(!silent)
        return 0;

    if( daemon(0, 1) != 0) {
        return -2;
    }
    return 0;
}

void print_usage(void) {
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

           "strategyht: --\n"
          "\t-sv, --svalue\t\tyesterday value of corresponding account\n"
          "\t-sp, --sposition\t\tmax position can hold\n"
          "\t-sl, --sloss\t\tmax loss can make\n"

           "fm2trader:\t\t-- Femas2.0 Trader app --\n"
          "\t-tf, --tfront\t\tset front address\n"
          "\t-tu, --tuser\t\tset user id \n"
          "\t-tp, --tpassword\t\tset password\n"
          "\t-tb, --tbroker\t\tset borker id\n"
          "\t-te, --texchange\t\tset exchange id\n"
           "\n"
           );
}

void unit_test(void) {
    int i;
    int j;
    const char* sym;
    void* addr;
    int len;
    IncQuotaDataT *dt = (IncQuotaDataT*)malloc(sizeof(IncQuotaData)*400);
    for(i=0; i<400; ++i) {
        memset(dt+i, 0, sizeof(IncQuotaDataT));

        if((i % 200) == 0) {
            strcpy(dt[i].InstTime.InstrumentID, trading_instrument);
        } else if((i % 99) == 0) {
            strcpy(dt[i].InstTime.InstrumentID, "rb1610");
        }

    }



    int cnt = 0;
    int64_t t1,t2;
    get_system_time(&t1);

    for(j=0; j<400; ++j) {
        addr = dt+j;
        len = sizeof(dt);
        sym = dt[j].InstTime.InstrumentID;

        if(strcmp(sym, trading_instrument) == 0){
            md_func(sym, addr, len);
            ++cnt;
        } else {
            for(i=0; i< array_ref_ins.size(); ++i) {
                if(strcmp(array_ref_ins[i].c_str(), sym ) == 0) {
                    md_func(sym, addr, len);
                    ++cnt;
                    break;
                }
            }
        }
    }
    get_system_time(&t2);
    lmice_critical_print("unit test time:%ld, count:%d\n", t2-t1, cnt);
    free(dt);
}

//void netmd_bf_create(void) {
//    uint64_t n = MAX_KEY_LENGTH;
//    uint32_t m = 0;
//    uint32_t k = 0;
//    double f = 0.0001;
//    /* MAX_KEY_LENGTH items, and with 0.0001 false positive */
//    eal_bf_calculate(n, f, &m, &k, &f);

//    bflter = (lm_bloomfilter_t*)malloc(sizeof(lm_bloomfilter_t)+m);
//    memset(bflter, 0, sizeof(lm_bloomfilter_t)+m);
//    bflter->n = n;
//    bflter->f = f;
//    bflter->m = m;
//    bflter->k = k;
//    bflter->addr = (char*)bflter+ sizeof(lm_bloomfilter_t);

//    lmice_critical_print("Bloomfilter initialized:\n\tn:%lu\n\tm:%u\n\tk:%u\n\tf:%5.15lf\n",
//                         n,m,k,f);

//}

//void netmd_bf_delete(void) {
//    free(bflter);
//    bflter = NULL;
//}


/* netmd worker thread */
int netmd_pcap_thread(const char *devname, const char* packet_filter) {
    pcap_t *adhandle;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_int netmask =0xffffff;
    /* setup the package filter  */
    /* const char packet_filter[] = "udp and port 30100"; */
    struct bpf_program fcode;

    /* Open the adapter */
    if ( (adhandle= pcap_open_live(devname, // name of the device
                                   65536, // portion of the packet to capture.
                                   // 65536 grants that the whole packet will be captured on all the MACs.
                                   1, // promiscuous mode
                                   1000, // read timeout
                                   errbuf // error buffer
                                   ) ) == NULL)
    {
        lmice_error_print("\nUnable to open the adapter[%s], as %s\n", devname, errbuf);
        return -1;
    } else {
        lmice_info_print("pcap_open_live done\n");
    }

    /*set buffer size 1MB, use default */
    pcap_set_buffer_size(adhandle, 1024*1024);


    /*compile the filter*/
    if(pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) <0 ){
        lmice_error_print("\nUnable to compile the packet filter. Check the syntax.\n");
        return -1;
    }
    /*set the filter*/
    if(pcap_setfilter(adhandle, &fcode)<0){
        lmice_error_print("\nError setting the filter.\n");
        return -1;
    }

    pcapHandle = adhandle;

    /* start the capture */
    lmice_critical_print("pcap startting...\n");
    pcap_loop(adhandle, -1, netmd_pcap_callback, (u_char*)0);
    lmice_critical_print("pcap stopped.\n");
}

void netmd_pcap_callback(u_char *arg, const struct pcap_pkthdr* pkthdr,const u_char* packet) {
    const char* data = (const char*)(packet+42);

    (void)pkthdr;
    (void)arg;
    // callq  4e990 <_ZN12CFTDCPackage14PreparePackageEjhh>

    g_pkg_time.tv_sec = pkthdr->ts.tv_sec;
    g_pkg_time.tv_usec = pkthdr->ts.tv_usec;


    /** pub data */
    netmd_pub_data( data + position, data, pkthdr->len - 42);


}

void netmd_pcap_stop(int sig) {
    if(pcapHandle) {
        pcap_breakloop(pcapHandle);
        pcapHandle = NULL;
    }
}

//static int key_compare(const void* key, const void* obj) {
//    const uint64_t* hval = (const uint64_t*)key;
//    const uint64_t* val = (const uint64_t*)obj;

//    if(*hval == *val)
//        return 0;
//    else if(*hval < *val)
//        return -1;
//    else
//        return 1;
//}

//forceinline int key_find_or_create(const char* symbol) {
//    uint64_t hval;
//    uint64_t *key = NULL;
//    hval = eal_hash64_fnv1a(symbol, 32);

//    if(keypos == 0) {
//        keylist[keypos] = hval;
//        ++keypos;
//        return 1;
//    }

//    key = (uint64_t*)bsearch(&hval, keylist, keypos, 8, key_compare);
//    if(key == NULL) {
//        /* create new element */
//        if(keypos < MAX_KEY_LENGTH) {
//            keylist[keypos] = hval;
//            ++keypos;
//            qsort(keylist, keypos, 8, key_compare);
//            return 1;
//        } else {
//            lmice_warning_print("Add key[%s] failed as list is full\n", symbol);
//            return -1;
//        }
//    }

//    return 0;

//}

forceinline void netmd_pub_data(const char* sym, const void* addr, int len) {
    size_t i;

    if(g_spi->status() != FMTRADER_LOGIN) {
        return;
    }
    get_system_time(&g_begin_time);

    if(strcmp(sym, trading_instrument) == 0){
       // lmice_critical_print("md_func %s\n", sym);

        md_func(sym, addr, len);
    } else {
        for(i=0; i< array_ref_ins.size(); ++i) {
            if(strcmp(array_ref_ins[i].c_str(), sym ) == 0) {

                md_func(sym, addr, len);
                //lmice_critical_print("md_func %s\n", sym);
                break;
            }
        }
    }

}
