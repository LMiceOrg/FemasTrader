#ifndef _POLICY_INS_H
#define _POLICY_INS_H

#define USE_CPLUS_LIB

#include "lmice_eal_common.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_time.h"
#include "lmice_trace.h" 
#include "guavaproto.h"

#ifdef USE_C_LIB
#include "fforecaster.h"
#endif
#ifdef USE_CPLUS_LIB
#include "forecaster.h"
#endif
#include "lmspi.h"
#include "strategy_status.h"
#include "fm3in1.h"


#define SPI_SYMBOL_SIZE 64
#define SPI_SYMBOL_PUB_COUNT 16



#define ST_TIME_LOG_MAX 65535 
enum ST_TIME_TYPE{
	st_time_get_md = 0,
	st_time_get_forecast,
	st_time_send_order
};

void md_func(const char* symbol, const void* addr, int size);
void status_func(const char* symbol, const void* addr, int size);

class strategy_ins
{
public:
    strategy_ins( STRATEGY_CONF_P ptr_conf, CLMSpi *spi );
	~strategy_ins();

public:	

	//初始化实例
	void init();

	//启动实例
	void run();

	//暂停实例
//	void pause();

	//退出实例，退出前清理所有现场
	void exit();

	//退出实例，不清理任何内容
	void _exit();	

	//设置超时动作
	int set_timeout();

	CLMSpi * get_spi(){ return m_strategy;}
#ifdef USE_CPLUS_LIB
	Forecaster* get_forecaster(){ return  m_forecaster;}
#endif	
	int get_pause_status() { return m_pause_flag; }
	int set_pause_status( int pause_status ) { m_pause_flag = pause_status; }

	int set_exit() { m_exit_flag = 1; }

public:
    //char m_spi_symbol[SPI_SYMBOL_PUB_COUNT][SPI_SYMBOL_SIZE];
    //CUR_STATUS_P m_status;
    //STRATEGY_CONF_P m_ins_conf;


private:
	CLMSpi *m_strategy;
	int m_exit_flag;
	int m_pause_flag;
	
#ifdef USE_CPLUS_LIB
	Forecaster *m_forecaster;
#endif

};

#endif
