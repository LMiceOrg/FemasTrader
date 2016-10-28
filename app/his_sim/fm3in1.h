#ifndef FM3IN1_H
#define FM3IN1_H
#include "strategy_status.h"
#define MODEL_NAME_SIZE 32
#define MAX_ORDER_ACTION 350//480

#include "fm2spi.h"
#include "lmice_eal_thread.h"
#include "ChinaL1DiscreteFeature.h"
#include <string>
#include <vector>

typedef struct strategy_conf{
    char m_model_name[MODEL_NAME_SIZE];
    unsigned int m_max_pos;
    double m_max_loss;
    double m_close_value;
}STRATEGY_CONF,*STRATEGY_CONF_P;

extern CFemas2TraderSpi * g_spi;

extern volatile int g_guava_quit_flag;

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

extern double g_thresh_num;
extern int g_active_buy_orders;
extern int g_active_sell_orders;
extern double g_order_buy_price;
extern double g_order_sell_price;

extern int g_done_flag;


extern char log_buffer[];
extern int log_pos;
extern char stastic_buffer[];
extern int stastic_pos;
extern char signal_log_buffer[];
extern int signal_log_pos;
extern int signal_log_line;

extern double session_fee;
extern int session_trading_times;
extern double g_signal_multiplier;

extern std::vector<std::string> array_feature_name;
	
#endif // FM3IN1_H
