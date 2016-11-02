#ifndef FM3IN1_HT_H
#define FM3IN1_HT_H

#include "guavaproto.h"
#include "strategy_status.h"
#include "ChinaL1DiscreteFeature.h"
#include "forecaster.h"

#include "USTPFtdcTraderApi.h"
#include "USTPFtdcUserApiStruct.h"

#include <stdint.h>
#include <float.h>
#include <sys/time.h>
#include <string.h>

#define FM_ACCOUNT_GAP 8
#define FM_ACCOUNT_DIFF 10000
#define FM_LOCALID_MULTIPLE 1000000

#define MODEL_NAME_SIZE 32
typedef struct strategy_conf{
    //char m_model_name[MODEL_NAME_SIZE];
    int  m_max_pos;
    double m_max_loss;
    double m_close_value;
}STRATEGY_CONF,*STRATEGY_CONF_P;

struct fm3_config_s {
    int silent;
    int test;
    int position;
    int bytes;
    /* pcap */
    //char devname[64];
    //char filter[128];
    char trading_instrument[32];
    /* trader */
    char    front_address[64];
    char    user[64];
    char    password[64];
    char    broker[64];
    char    investor[64];
    char    exchange_id[32];
    char    md_name[32];
    int     account_id;
    struct itimerval flatten_tick;
    /* MD */
    int     mc_port;
    char    mc_group[32];
    char    mc_bindip[32];

};
typedef struct fm3_config_s fm3_config_t;

enum fm3_enums {
    FM3_RECV_BUFF_SIZE = 1024,
    FM3_INSTRUMENT_SIZE = 8,
};

struct fm3_args_t {

    fm3_config_t config;
    union {
        char line[FM3_RECV_BUFF_SIZE];
        SHFEQuotDataTag data;
    };
    STRATEGY_CONF               st_conf;
    CUR_STATUS                  cur_status;
    CUstpFtdcInputOrderField    order;
    CUstpFtdcOrderActionField   order_action;
    Forecaster* forecaster;
    int64_t tags[FM3_INSTRUMENT_SIZE];
    Dummy_ChinaL1Msg msgs[FM3_INSTRUMENT_SIZE];
    int n_rcved;
    int order_action_count;
    volatile int guava_quit_flag;
    volatile int orderaction_flag;
    /* time perf */
    int64_t begin_time;
    int64_t proc_time;
    int64_t end_time;
    char cur_sysid[32];
    fm3_args_t() {
        memset(&config, 0, sizeof(config));
        memset(line, 0, sizeof(line));
        memset(&st_conf, 0, sizeof(st_conf));
        memset(&cur_status, 0, sizeof(cur_status));
        memset(&order, 0, sizeof(order));
        memset(&order_action, 0, sizeof(order_action));
        forecaster = 0;
        for(int i=0; i<FM3_INSTRUMENT_SIZE; ++i) {
            tags[i]=0;
        }
        n_rcved = 0;
        order_action_count =0;
        guava_quit_flag=0;
        orderaction_flag=0;
        begin_time =0;
        proc_time =0;
        end_time = 0;
        memset(cur_sysid, 0, sizeof(cur_sysid));
    }
};



class CFemas2TraderSpi;
extern CFemas2TraderSpi* g_spi;
extern volatile int* g_orderaction_flag;
extern volatile int* g_flatten_flag;
#endif /** FM3IN1_HT_H */
