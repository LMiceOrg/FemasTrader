#ifndef FM3IN1_H
#define FM3IN1_H
#include "strategy_status.h"
#define MODEL_NAME_SIZE 32

#include "fm2spi.h"
#include "lmice_eal_thread.h"
#include "ChinaL1DiscreteFeature.h"

#define FM_ACCOUNT_GAP 8
#define FM_ACCOUNT_DIFF 10000
#define FM_LOCALID_MULTIPLE 1000000

typedef struct strategy_conf{
    char m_model_name[MODEL_NAME_SIZE];
    unsigned int m_max_pos;
    double m_max_loss;
    double m_close_value;
}STRATEGY_CONF,*STRATEGY_CONF_P;

extern CFemas2TraderSpi * g_spi;

class strategy_ins;
extern strategy_ins *g_ins;

extern CUR_STATUS g_cur_status;
extern CUR_STATUS_P g_cur_st; //&g_cur_status;
extern STRATEGY_CONF st_conf;
extern STRATEGY_CONF_P g_conf; //= &st_conf;
extern CUstpFtdcInputOrderField g_order;
extern CUstpFtdcOrderActionField g_order_action;
extern int g_order_action_count;
extern int64_t g_begin_time;
extern int64_t g_end_time;
extern struct timeval g_pkg_time;
extern ChinaL1Msg msg_data;
extern char trading_instrument[32];
extern struct itimerval g_tick;
extern char g_cur_sysid[31];
extern double g_trade_buy_price;
extern double g_trade_sell_price;
extern double g_flatten_threshold;
extern int g_done_flag;

extern int g_order_direction;
extern int g_active_buy_orders;
extern int g_active_sell_orders;


extern void order_action_func(int signo);
extern void uninit_time();

extern int g_flatten_flag;
extern int g_account_id;

#endif // FM3IN1_H
