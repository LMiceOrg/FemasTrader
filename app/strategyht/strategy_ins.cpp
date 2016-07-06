
#include "strategy_ins.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <float.h>

#include <unistd.h>
#include <sys/types.h>

using namespace std;

extern strategy_ins *g_ins;
int g_exit_flag = 0;

/*
int g_init_flag = 0;
strategy_status *g_status = NULL;
CLMSpi *g_strategy_status = NULL;
CLMSpi *g_strategy_md = NULL;
double g_maxloss = 0;
double g_max_position = 0;
int g_exit_flag = 0;
int g_trader_flag = 1;
*/

#define MAX_SIZE 512
int get_conf_path( char *path )
{
	char *current_absolute_path = path;
	//获取当前程序绝对路径 
	int cnt = readlink("/proc/self/exe", current_absolute_path, MAX_SIZE); 

	if( cnt <= 0 )
	{
		return -1;
	}

	//获取当前目录绝对路径，即去掉程序名 
	for (int i = cnt; i >=0; --i) 
	{ 
	    if (current_absolute_path[i] == '/') 
	    { 
	        current_absolute_path[i+1] = '\0'; 
	        break; 
	    } 
	} 
	strcat( current_absolute_path, CONF_FILE_NAME );

	return 0;
}

strategy_ins::strategy_ins()
{

	//g_max_position = m_ins_conf.maxposition;
	//m_status = new strategy_status( m_ins_conf.instrument );
	//g_status = m_status;

	//m_strategy_status = new CLMSpi("strategy-test-status");
	//g_strategy_status = m_strategy_status;
	init();

	//g_strategy_md = m_strategy_md;
}

strategy_ins::~strategy_ins()
{
	//delete m_status;
	//delete m_strategy_status;
	delete m_strategy_md;
	delete m_forecaster;
}

#define MAX_SIZE 256
void strategy_ins::get_conf()
{
	char current_absolute_path[MAX_SIZE];
	memset( current_absolute_path, 0, MAX_SIZE );
	memset( &m_ins_conf, 0, sizeof(INS_CONF) );
	get_conf_path( current_absolute_path );

	//read conf file, to be add

	memset(m_ins_conf.model_name, 0, sizeof(m_ins_conf.model_name));
	memset(m_ins_conf.instrument, 0, sizeof(m_ins_conf.instrument));
	strcpy( m_ins_conf.model_name, "strategy_hc0");
	strcpy( m_ins_conf.instrument, "hc1610" ); 
	m_ins_conf.ins_factor = 1;
	m_ins_conf.maxloss_factor = 0.2;
	m_ins_conf.maxloss = 0;
	m_ins_conf.maxposition = 1;

	//DEBUG_LOG("get config file");
}

/*
void status_func(const char* symbol, const void* addr, uint32_t size)
{
    if( strcmp(symbol, "[trader-test]-tracker-m-position") && size >= sizeof(struct CUstpFtdcRspInvestorPositionField) )
    {
		struct CUstpFtdcRspInvestorPositionField *serv_pos = (struct CUstpFtdcRspInvestorPositionField *)addr;
		g_status->set_pos(serv_pos);
	}
	else if( strcmp(symbol, "[trader-test]-tracker-m-account") && size >= sizeof(struct CUstpFtdcRspInvestorAccountField) )
	{
		struct CUstpFtdcRspInvestorAccountField *serv_ac = (struct CUstpFtdcRspInvestorAccountField *)addr;
		g_status->set_ac(serv_ac);
	}
	else if( strcmp(symbol, "[trader-test]-tracker-PL") && size >= sizeof(TRPL) )
	{
		PTRPL serv_pl = (PTRPL) addr;
		if( g_status->set_pl(serv_pl) < g_maxloss )
		{
			g_ins->ins_exit();
		}
	}
	else
	{
		DEBUG_LOG("symbol error!");
	}

	return;
}
*/

void strategy_ins::init()
{
	//m_strategy_status->subscribe("[trader-test]-tracker-m-position");
	//m_strategy_status->subscribe("[trader-test]-tracker-m-account");
	//m_strategy_status->subscribe("[trader-test]-tracker-PL");
	//m_strategy_status->register_callback(status_func);
	lmice_info_print("get conf file:\n");
	memset( &m_ins_conf, 0, sizeof(INS_CONF) );
	get_conf();
	lmice_info_print("model name: %s\n ins_factor:%d\n max_position:%d\n max_loss:%d\n", m_ins_conf.model_name, m_ins_conf.ins_factor, m_ins_conf.maxposition, m_ins_conf.maxloss);

	m_strategy_md = new CLMSpi("strategy-md");

	string type = "hc_0";
	string date = "";
	m_forecaster = ForecasterFactory::createForecaster( type, date );
}

//static double g_last_price = 0;

void md_func(const char* symbol, const void* addr, int size)
{
/*
    if( ! g_trader_flag )
    {
		return;
	}
*/
	if( size < sizeof(IncQuotaDataT))
	{
		return;
	}

	Forecaster *fc = g_ins->get_forecaster();
	
	pIncQuotaDataT md_data = (pIncQuotaDataT) addr;
	lmice_debug_print("get netmd: {name:%s}, {time: %s-%d}, {lastprice:%s}\n", md_data->InstTime.InstrumentID, md_data->InstTime.UpdateTime, md_data->InstTime.UpdateMillisec ,md_data->PriVol.LastPrice);


	if( md_data->AskBidInst.AskPrice1 == DBL_MAX || md_data->AskBidInst.BidPrice1 == DBL_MAX || md_data->PriVol.LastPrice == DBL_MAX )
	{
		return;
	}

	//translate format to ChinaL1Msg
	ChinaL1Msg msg_data;
	msg_data.set_inst( md_data->InstTime.InstrumentID );

	int64_t micro_time = 0;
	time_t data_time1, current;
	current = time(NULL);
	struct tm t = *gmtime(&current);

	char data_time[16];
	memset(data_time, 0, 16);
	strcpy(data_time, md_data->InstTime.UpdateTime);

	char tmp[8];
	memset(tmp, 0, 8);
	memcpy( tmp, data_time, 2 );
	t.tm_hour = atoi(tmp);
	memcpy( tmp, data_time+3, 2 );
	t.tm_min = atoi(tmp);
	memcpy( tmp, data_time+6, 2 );
	t.tm_sec = atoi(tmp);
	data_time1 = mktime(&t);
	micro_time = (int64_t)data_time1 * 1000 * 1000;
	micro_time += md_data->InstTime.UpdateMillisec;
	msg_data.set_time( micro_time );
	msg_data.set_bid( md_data->AskBidInst.BidPrice1 );
	msg_data.set_offer( md_data->AskBidInst.AskPrice1 );
	msg_data.set_bid_quantity( md_data->AskBidInst.BidVolume1 );
	msg_data.set_offer_quantity( md_data->AskBidInst.AskVolume1 );
	msg_data.set_volume( md_data->PriVol.Volume );
	msg_data.set_notinal( md_data->PriVol.Turnover );
	msg_data.set_limit_up( 0 );
	msg_data.set_limit_down( 0 );

	fc->update(msg_data);
	double signal = fc->get_forecast();
	double mid = 0.5 * ( md_data->AskBidInst.AskPrice1 + md_data->AskBidInst.BidPrice1 );
	double forecast = mid * ( 1 + signal );

	lmice_debug_print("forecast: %llf\n", forecast);
	
	//int position = g_status->get_pos();

    //send order insert command
	CUstpFtdcInputOrderField *orderTest = new CUstpFtdcInputOrderField;
	memset( orderTest, 0, sizeof(CUstpFtdcInputOrderField) );

	strcpy(orderTest->BrokerID, "0129");
	strcpy(orderTest->UserID, "012981073650");
	strcpy(orderTest->ExchangeID, "SHFE");
	strcpy(orderTest->InstrumentID, g_ins->m_ins_conf.instrument);
	strcpy(orderTest->InvestorID, "81073650");
	orderTest->OrderPriceType = USTP_FTDC_OPT_LimitPrice;
	orderTest->OffsetFlag = USTP_FTDC_OF_Open;
	orderTest->HedgeFlag = USTP_FTDC_CHF_Speculation;
	orderTest->Volume = 1;
	orderTest->TimeCondition = USTP_FTDC_TC_IOC;
	orderTest->VolumeCondition = USTP_FTDC_VC_AV;
	orderTest->ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
		
	int ordersize = 1;
	double orderPrice = 0;
	if( forecast > md_data->AskBidInst.AskPrice1 )
	{
		//int ordersize = min( md_data->AskBidInst.AskVolume1, g_max_position - position);
		orderTest->LimitPrice = md_data->AskBidInst.AskPrice1;
		orderTest->Direction = USTP_FTDC_D_Sell;
		
	}
	if( forecast < md_data->AskBidInst.BidPrice1 )
	{
		//int ordersize = min( md_data->AskBidInst.BidVolume1, g_max_position + position);
		orderTest->LimitPrice = md_data->AskBidInst.BidPrice1;
		orderTest->Direction = USTP_FTDC_D_Buy;
	}

	//send orderinster
//	g_exit_flag++;
//	CLMSpi *spi = g_ins->get_spi();
//	spi->send("[strategy]-req-orderInster", orderTest, sizeof(CUstpFtdcInputOrderField));
	
}

void strategy_ins::run()
{
/*
	DEBUG_LOG("waiting for init finished");
	while( g_init_flag != 0x11 )
	{
		usleep(500);
	}

	g_maxloss = -1 * m_ins_conf.maxloss_factor * m_status->get_open_ac();

	DEBUG_LOG("instrument:%s", m_ins_conf->instrument);
	DEBUG_LOG("instrument factor:%f", m_ins_conf->ins_factor);
	DEBUG_LOG("max position:%d", m_ins_conf->maxposition);
	DEBUG_LOG("current position:%d", m_status->get_pos());
	DEBUG_LOG("valid fund:%f", m_status->get_open_ac());
	DEBUG_LOG("maxloss:%f", g_maxloss);
*/
	//m_strategy_md->publish("[strategy]-req-flatten");

/*
	//设置超时时间，时间到达后发出清仓指令
	if( set_timeout() < 0 )
	{
		DEBUG_LOG("start time error, exit!");
		ins_exit();
		return;
	}

	DEBUG_LOG("start running");
	*/
	lmice_info_print("publish symbol: [strategy]-req-orderInster\n");
	m_strategy_md->publish("[strategy]-req-orderInster");

	char instrument[16];
	memset(instrument, 0, 16);
	sprintf(instrument, "[netmd]%s", m_ins_conf.instrument);
	lmice_info_print("subscribe symbol:[netmd]%s\n", instrument);
	m_strategy_md->subscribe(instrument);
	memset(instrument, 0, 16);
	sprintf(instrument, "[netmd]%s", "rb1610");
	m_strategy_md->subscribe(instrument);
	lmice_info_print("subscribe symbol:[netmd]%s\n", instrument);
	
	m_strategy_md->register_callback(md_func, NULL);

	while(g_exit_flag < 2)
	{
		usleep(50000);
	}
	
}

/*
int strategy_ins::set_timeout()
{
	time_t endline1, endline2, current,span;
	current = time(NULL);
	struct tm t = *gmtime(&current);
	t.tm_hour = 11;
	t.tm_min = 25;
	t.tm_sec = 0;
	endline1 = mktime(&t);
	
	t.tm_hour = 14;
	t.tm_min = 55;
	t.tm_sec = 0;
	endline2 = mktime(&t);

	if( current < endline1 )
	{
		span = endline1 - current;
	}
	else if( current < endline2 )
	{
		span = endline2 - current;
	}
	else
	{
		return -1;
	}

	alarm(span);

}
*/

void strategy_ins::ins_exit()
{
/*
	DEBUG_LOG("flatten and exit");

	g_trader_flag = 0;
	if( m_strategy_md != NULL )
	{
		double price = m_status->get_md();
		m_strategy_md->send2("[strategy-test]-req-flatten", &price, sizeof(price));
	}
	sleep(5);
	
	g_exit_flag = 1;
	*/
}

void strategy_ins::_ins_exit()
{
/*
	DEBUG_LOG("exit");
	g_trader_flag = 0;
	g_exit_flag = 1;
	*/
}



