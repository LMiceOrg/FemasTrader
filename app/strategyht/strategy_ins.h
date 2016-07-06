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
	//加载配置文件
	void get_conf();

	//初始化实例
	void init();

	//启动实例
	void run();

	//暂停实例
//	void pause();

	//退出实例，退出前清理所有现场
	void ins_exit();

	//退出实例，不清理任何内容
	void _ins_exit();	

	//设置超时动作
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
