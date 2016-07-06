#ifndef _POLICY_INS_H
#define _POLICY_INS_H

#include "lmice_eal_common.h"
#include "lmice_eal_thread.h"
#include "lmice_eal_time.h"
#include "lmice_trace.h" 


#include "forecaster.h"

#include "struct_ht.h"
//#include "strategy_status.h"
#include "lmspi.h"

#define CONF_FILE_NAME "strategy_ht.ini"
/*
#define DEBUG_LOG( format, ... ) do{ \
	int64_t time = 0; \
	get_system_time(&time); \
	char log_format[512]; \
	memset(log_format, 0 , 512); \
	strcpy(log_format, "(%s)-(%d)-(%s)-content:" ); \
	strcat(log_format, format);\
	strcat(log_format, "\n");
	printf(log_format, m_ins_conf.model_name, time, __FUNCTION__, ##__VA_ARGS__ ); \
}while(0) */

typedef struct ins_conf{
	char model_name[32];
	char instrument[16];
	double ins_factor;
	unsigned int maxposition;
	double maxloss;
	double maxloss_factor;
}INS_CONF;

class strategy_ins
{

public:
	strategy_ins();
	~strategy_ins();

public:	
	//���������ļ�
	void get_conf();

	//��ʼ��ʵ��
	void init();

	//����ʵ��
	void run();

	//��ͣʵ��
//	void pause();

	//�˳�ʵ�����˳�ǰ���������ֳ�
	void ins_exit();

	//�˳�ʵ�����������κ�����
	void _ins_exit();	

	//���ó�ʱ����
	int set_timeout();

	Forecaster* get_forecaster(){ return  m_forecaster;}
	CLMSpi * get_spi(){ return m_strategy_md;}
	

	INS_CONF m_ins_conf;

private:
//	strategy_status *m_status;
//	CLMSpi *m_strategy_status;
	CLMSpi *m_strategy_md;
	Forecaster *m_forecaster;
};

#endif
