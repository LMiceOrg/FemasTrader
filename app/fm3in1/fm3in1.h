#ifndef FM3IN1_H
#define FM3IN1_H
#include "strategy_status.h"
#define MODEL_NAME_SIZE 32

#include "fm2spi.h"
#include "lmice_eal_thread.h"

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
extern int64_t g_begin_time;
extern int64_t g_end_time;
extern struct timeval g_pkg_time;



#endif // FM3IN1_H
