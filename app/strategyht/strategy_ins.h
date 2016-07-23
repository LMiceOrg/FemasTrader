#ifndef _POLICY_INS_H
#define _POLICY_INS_H

#include "lmice_eal_common.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_time.h"
#include "lmice_trace.h" 
#include "fforecaster.h"
#include "struct_ht.h"
#include "lmspi.h"
#include "strategy_status.h"

#define MODEL_NAME_SIZE 32
#define SPI_SYMBOL_SIZE 64
#define SPI_SYMBOL_PUB_COUNT 16

typedef struct strategy_conf{
	char m_model_name[MODEL_NAME_SIZE];
	unsigned int m_max_pos;
	double m_max_loss;
	double m_close_value;
}STRATEGY_CONF,*STRATEGY_CONF_P;

class strategy_ins
{
public:
	strategy_ins( STRATEGY_CONF_P ptr_conf );
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
	//const CUR_STATUS_P get_status(){ return m_status; }
	
	int get_pause_status() { return m_pause_flag; }
	int set_pause_status( int pause_status ) { m_pause_flag = pause_status; }

	int set_exit() { m_exit_flag = 1; }

public:
	char m_spi_symbol[SPI_SYMBOL_PUB_COUNT][SPI_SYMBOL_SIZE];
	CUR_STATUS_P m_status;
	STRATEGY_CONF_P m_ins_conf;

private:
	CLMSpi *m_strategy;
	int m_exit_flag;
	int m_pause_flag;

};

#endif
