#ifndef _POLICY_INS_H
#define _POLICY_INS_H

#define USE_CPLUS_LIB

#include "lmice_eal_common.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_time.h"
#include "lmice_trace.h" 
#include "struct_ht.h"

#ifdef USE_C_LIB
#include "fforecaster.h"
#endif
#ifdef USE_CPLUS_LIB
#include "forecaster.h"
#endif
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

#define ST_TIME_LOG_MAX 65535 
enum ST_TIME_TYPE{
	st_time_get_md = 0,
	st_time_get_forecast,
	st_time_send_order
};

class strategy_ins
{
public:
	strategy_ins( STRATEGY_CONF_P ptr_conf );
	~strategy_ins();

public:	

	//��ʼ��ʵ��
	void init();

	//����ʵ��
	void run();

	//��ͣʵ��
//	void pause();

	//�˳�ʵ�����˳�ǰ���������ֳ�
	void exit();

	//�˳�ʵ�����������κ�����
	void _exit();	

	//���ó�ʱ����
	int set_timeout();

	CLMSpi * get_spi(){ return m_strategy;}
#ifdef USE_CPLUS_LIB
	Forecaster* get_forecaster(){ return  m_forecaster;}
#endif	
	int get_pause_status() { return m_pause_flag; }
	int set_pause_status( int pause_status ) { m_pause_flag = pause_status; }

	int set_exit() { m_exit_flag = 1; }

public:
	char m_spi_symbol[SPI_SYMBOL_PUB_COUNT][SPI_SYMBOL_SIZE];
	CUR_STATUS_P m_status;
	STRATEGY_CONF_P m_ins_conf;
	int64_t m_time_log[ST_TIME_LOG_MAX][3];
	unsigned long m_time_log_pos;

private:
	CLMSpi *m_strategy;
	int m_exit_flag;
	int m_pause_flag;
	
#ifdef USE_CPLUS_LIB
	Forecaster *m_forecaster;
#endif

};

#endif
