
#include "strategy_ins.h"
#include "lmice_eal_bson.h"
#include "lmice_eal_time.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <float.h>

#include <unistd.h>
#include <sys/types.h>

#define FC_DEBUG 1

enum spi_symbol{
	//publish symbol
	symbol_order_insert = 0,
	symbol_order_action,
	symbol_flatten,
	symbol_hard_flatten,
	symbol_log,
	//subscribe symbol
	symbol_position,
	symbol_account
};

using namespace std;

extern strategy_ins *g_ins;

CLMSpi * g_spi = NULL;
CUR_STATUS_P g_cur_st = NULL;
STRATEGY_CONF_P g_conf = NULL;
CUstpFtdcInputOrderField g_order;

#define MAX_LOG_SIZE 1024

//call back function 
void md_func(const char* symbol, const void* addr, int size)
{
	char str_log[MAX_LOG_SIZE];
	int64_t systime_md = 0;
	get_system_time(&systime_md);

	if( 1 == g_ins->get_pause_status() )
	{
		return;
	}

	if( size < sizeof(IncQuotaDataT))
	{
		return;
	}

	pIncQuotaDataT md_data = (pIncQuotaDataT) addr;

	if( md_data->AskBidInst.AskPrice1 == DBL_MAX || md_data->AskBidInst.BidPrice1 == DBL_MAX || md_data->PriVol.LastPrice == DBL_MAX )
	{
		return;
	}

	memset(str_log, 0, sizeof(str_log));
	sprintf( str_log, "[%ld]get md data,content: %s\n", systime_md, md_data->InstTime.InstrumentID );
	g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );

	//translate format to FC_MD_MSG
	FC_MD_MSG md_msg;
	memset( &md_msg, 0, sizeof(FC_MD_MSG) );

	strcpy( md_msg.m_inst, md_data->InstTime.InstrumentID );
	
	int64_t micro_time = 0;
	time_t data_time1, current;
	current = time(NULL);
	struct tm t = *localtime(&current);

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
	micro_time += md_data->InstTime.UpdateMillisec * 1000;

	md_msg.m_time_micro = micro_time;
	
	md_msg.m_bid = md_data->AskBidInst.BidPrice1;
	md_msg.m_offer =  md_data->AskBidInst.AskPrice1;
	md_msg.m_bid_quantity = md_data->AskBidInst.BidVolume1;
	md_msg.m_offer_quantity = md_data->AskBidInst.AskVolume1;
	md_msg.m_volume = md_data->PriVol.Volume;
	md_msg.m_notional = md_data->PriVol.Turnover;
	md_msg.m_limit_up = 0;
	md_msg.m_limit_down = 0;

	get_system_time(&systime_md);
	memset(str_log, 0, sizeof(str_log));
	sprintf( str_log, "[%ld]update to forecaster,content: %s\n", systime_md, md_msg.m_inst );
	g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );
	
	fc_update_msg( &md_msg );
	double signal = fc_get_forecast();

#ifdef FC_DEBUG	
	double arr_signal[12];
	char arr_name[16][64];
	int sig_size = 0;
	memset( arr_signal, 0, sizeof(arr_signal) );
	memset( arr_name, 0, sizeof(arr_name) );
	fc_get_all_signal( arr_name, arr_signal, &sig_size);
#endif

	if( 0 == strcmp( md_data->InstTime.InstrumentID, g_cur_st->m_ins_name ) )
	{

		if( md_data->PriVol.LastPrice > 0 && md_data->PriVol.LastPrice != g_cur_st->m_md.m_last_price)
		{
			g_cur_st->m_md.m_last_price =  md_data->PriVol.LastPrice;
			int pl = ( g_cur_st->m_pos.m_buy_count + g_cur_st->m_pos.m_sell_count) * g_cur_st->m_md.m_last_price + g_cur_st->m_acc.m_valid_money - g_conf->m_close_value;
			if( -pl >= g_conf->m_max_loss )
			{
				g_ins->exit();
				lmice_critical_print("touch max loss,stop and exit\n");
			}
		}

		double mid = 0.5 * ( md_data->AskBidInst.AskPrice1 + md_data->AskBidInst.BidPrice1 );
		double forecast = mid * ( 1 + signal );

#ifdef FC_DEBUG
		int64_t systime_fc = 0;
		get_system_time(&systime_fc);

		EalBson bson_text, bson_msg, bson_fc;
		bson_msg.AppendInt64("recv_time:", systime_md);
		bson_msg.AppendSymbol("m_inst", md_msg.m_inst);
		bson_msg.AppendInt64("m_time_micro", md_msg.m_time_micro);
		bson_msg.AppendDouble("m_offer", md_msg.m_offer);
		bson_msg.AppendInt32("m_offer_quantity", md_msg.m_offer_quantity);
		bson_msg.AppendDouble("m_bid", md_msg.m_bid);
		bson_msg.AppendInt32("m_bidr_quantity", md_msg.m_bid_quantity);
		bson_msg.AppendInt32("m_volume", md_msg.m_volume);
		bson_msg.AppendDouble("m_notional", md_msg.m_notional);
		bson_msg.AppendDouble("m_limit_down", md_msg.m_limit_down);
		bson_msg.AppendDouble("m_limit_up", md_msg.m_limit_up);

		bson_fc.AppendInt64("forecaster time:", systime_fc);
		bson_fc.AppendDouble("final_signal:", signal);
		bson_fc.AppendDouble("ChinaL1DiscreteSelfDecayingReturnFeature1", arr_signal[0]);
		bson_fc.AppendDouble("ChinaL1DiscreteSelfDecayingReturnFeature2", arr_signal[1]);
		bson_fc.AppendDouble("ChinaL1DiscreteSelfDecayingReturnFeature3", arr_signal[2]);
		bson_fc.AppendDouble("ChinaL1DiscreteSelfBookImbalanceFeature", arr_signal[3]);
		bson_fc.AppendDouble("ChinaL1DiscreteSelfTradeImbalanceFeature", arr_signal[4]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherBestSinceFeature", arr_signal[5]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherDecayingReturnFeature1", arr_signal[6]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherDecayingReturnFeature2", arr_signal[7]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherDecayingReturnFeature3", arr_signal[8]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherBookImbalanceFeature1", arr_signal[9]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherBookImbalanceFeature2", arr_signal[10]);
		bson_fc.AppendDouble("ChinaL1DiscreteOtherBookImbalanceFeature3", arr_signal[11]);
		bson_fc.AppendDouble("final_forecaster", forecast);

		bson_text.AppendDocument( "md_msg", &bson_msg );
		bson_text.AppendDocument( "fc_msg", &bson_fc );
		const char *strJson = bson_text.GetJsonData();
		g_spi->send("fc_debug_logging", strJson, strlen(strJson));
		bson_text.FreeJsonData();
		return;
#endif

	    //send order insert command
		int ask_order_size = 0;
		int left_pos_size = 0;
		if( forecast > md_data->AskBidInst.AskPrice1 )
		{
			left_pos_size = g_conf->m_max_pos - g_cur_st->m_pos.m_buy_count;
			if( 0 == left_pos_size )
			{
				return;
			}
			ask_order_size = left_pos_size < md_data->AskBidInst.AskVolume1 ? left_pos_size : md_data->AskBidInst.AskVolume1;
			g_order.LimitPrice = md_data->AskBidInst.AskPrice1;

			if( g_cur_st->m_pos.m_sell_count > ask_order_size )
			{
				g_order.Direction = USTP_FTDC_D_Buy;
				g_order.OffsetFlag = USTP_FTDC_OF_Close;
			}
			else
			{
				g_order.Direction = USTP_FTDC_D_Buy;
				g_order.OffsetFlag = USTP_FTDC_OF_Open;
			}
		}
		
		if( forecast < md_data->AskBidInst.BidPrice1 )
		{
		  	left_pos_size = g_conf->m_max_pos - g_cur_st->m_pos.m_sell_count;
			if( 0 == left_pos_size )
			{
				return;
			}
			ask_order_size = left_pos_size < md_data->AskBidInst.BidVolume1 ?
					left_pos_size : md_data->AskBidInst.BidVolume1;
			g_order.LimitPrice = md_data->AskBidInst.BidPrice1;
			
			if( g_cur_st->m_pos.m_buy_count > ask_order_size )
			{
				g_order.Direction = USTP_FTDC_D_Sell;
				g_order.OffsetFlag = USTP_FTDC_OF_Close;
			}
			else
			{
				g_order.Direction = USTP_FTDC_D_Sell;
				g_order.OffsetFlag = USTP_FTDC_OF_Open;
			}
		}

		g_order.Volume = ask_order_size;
		get_system_time(&systime_md);
		memset(str_log, 0, sizeof(str_log));
		sprintf( str_log, "[%ld]send order insert,content: %s\n", systime_md, md_msg.m_inst );
		g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );

		//send order insert
		g_spi->send( g_ins->m_spi_symbol[symbol_order_insert], &g_order, sizeof(CUstpFtdcInputOrderField));


			
	}
}


void status_func(const char* symbol, const void* addr, int size)
{
	char str_log[MAX_LOG_SIZE];
	int64_t systime_md = 0;

	if( 1 == g_ins->get_pause_status() )
	{
		return;
	}

	if( 0 == strcmp( symbol, g_ins->m_spi_symbol[symbol_position] ) )
	{
		get_system_time(&systime_md);
		memset(str_log, 0, sizeof(str_log));
		sprintf( str_log, "[%ld]get position msg,content:\n", systime_md );
		g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );
		
		//to be add
	}
	else if( 0 == strcmp( symbol, g_ins->m_spi_symbol[symbol_account] ) )
	{
		get_system_time(&systime_md);
		memset(str_log, 0, sizeof(str_log));
		sprintf( str_log, "[%ld]get account msg,content:\n", systime_md );
		g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );
		//to be add
	}

	//calculate pl
	{
		int pl = ( g_cur_st->m_pos.m_buy_count + g_cur_st->m_pos.m_sell_count) * g_cur_st->m_md.m_last_price + g_cur_st->m_acc.m_valid_money - g_conf->m_close_value;
		if( -pl >= g_conf->m_max_loss )
		{
			g_ins->exit();
			lmice_critical_print("touch max loss,stop and exit\n");
		}
	}

	return;
}

//class function

strategy_ins::strategy_ins( STRATEGY_CONF_P ptr_conf ):m_exit_flag(0),m_pause_flag(0),m_ins_conf(ptr_conf)
{
	lmice_info_print("model name: %s\nmax positon: %d\nmax loss: %lf\nclose value: %lf\n",
                             m_ins_conf->m_model_name, m_ins_conf->m_max_pos, 
                             m_ins_conf->m_max_loss, m_ins_conf->m_close_value);	
	m_strategy = new CLMSpi(m_ins_conf->m_model_name, -1);	
	m_status = new CUR_STATUS;
	memset(m_status, 0, sizeof(CUR_STATUS));
	memset( m_spi_symbol, 0, sizeof(m_spi_symbol));

	g_spi = m_strategy;
	g_cur_st = m_status;
	g_conf = m_ins_conf;
	
	init();
}

strategy_ins::~strategy_ins()
{
	delete m_status;
	delete m_strategy;
}


void strategy_ins::init()
{	
	string type = "hc_0";
	time_t current;
	current = time(NULL);
	struct tm date = *localtime(&current);
	if ( fc_init( "hc_0", date ) < 0 )
	{
		lmice_error_print(" forecaster lib init error\n ");
	}

	sprintf( m_spi_symbol[symbol_order_insert], "[%s]%s", m_ins_conf->m_model_name, "OrderInsert" );
	sprintf( m_spi_symbol[symbol_flatten], "[%s]%s", m_ins_conf->m_model_name, "Flatten" );
	sprintf( m_spi_symbol[symbol_log], "[%s]%s", m_ins_conf->m_model_name, "Logging");
	sprintf( m_spi_symbol[symbol_position], "[%s]%s", m_ins_conf->m_model_name, "Position" );
	sprintf( m_spi_symbol[symbol_account], "[%s]%s", m_ins_conf->m_model_name, "Account" );

	memset( &g_order, 0, sizeof(CUstpFtdcInputOrderField));
	g_order.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
	g_order.HedgeFlag = USTP_FTDC_CHF_Speculation;
	g_order.TimeCondition = USTP_FTDC_TC_IOC;
	g_order.VolumeCondition = USTP_FTDC_VC_AV;
	g_order.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
	
	return;
}

void strategy_ins::run()
{

	//���ó�ʱʱ�䣬ʱ�䵽��󷢳����ָ��
	if( set_timeout() < 0 )
	{
		lmice_error_print("set time out signal error\n");
		return;
	}

	lmice_info_print( "start running\n");
	lmice_info_print( "publish symbol: %s\n", m_spi_symbol[symbol_flatten] );
	m_strategy->publish( m_spi_symbol[symbol_flatten] );
	
	lmice_info_print( "publish symbol: %s\n", m_spi_symbol[symbol_order_insert] );
	m_strategy->publish( m_spi_symbol[symbol_order_insert] );

	lmice_info_print( "publish symbol: %s\n", m_spi_symbol[symbol_log] );
	m_strategy->publish( m_spi_symbol[symbol_log] );

	int size = 0;
	char tmp_list[8][16];
	memset( tmp_list, 0, sizeof(tmp_list) );
	fc_get_trade_list( tmp_list ,&size );
	strcpy( m_status->m_ins_name, tmp_list[0] );

	char symbol[SPI_SYMBOL_SIZE];
	memset(symbol, 0, SPI_SYMBOL_SIZE);
	sprintf( symbol, "[%s]%s", m_ins_conf->m_model_name, m_status->m_ins_name );
	lmice_info_print( "subscribe symbol: %s\n", m_status->m_ins_name );
	m_strategy->subscribe( symbol );
	m_strategy->register_callback(md_func, symbol );

	memset( tmp_list, 0, sizeof(tmp_list) );
	size = 0;
	fc_get_ref_list( tmp_list, &size );
	int i;
	for( i=0; i<size; i++ )
	{
		memset(symbol, 0, SPI_SYMBOL_SIZE);
		sprintf( symbol, "[%s]%s", m_ins_conf->m_model_name, tmp_list[i] );
		lmice_info_print( "subscribe symbol: %s\n", symbol );
		m_strategy->subscribe( symbol );
		m_strategy->register_callback(md_func, symbol );
	}

	lmice_info_print( "subscribe symbol: %s\n", m_spi_symbol[symbol_position] );
	m_strategy->subscribe( m_spi_symbol[symbol_position] );
	m_strategy->register_callback( status_func, m_spi_symbol[symbol_position] );

	lmice_info_print( "subscribe symbol: %s\n", m_spi_symbol[symbol_account] );
	m_strategy->subscribe( m_spi_symbol[symbol_account] );
	m_strategy->register_callback( status_func, m_spi_symbol[symbol_account] );

#ifdef FC_DEBUG
	lmice_info_print( "public symbol: %s\n", "fc_debug_logging" );
	m_strategy->publish( "fc_debug_logging" );
#endif 

	m_strategy->join();
	m_strategy->delete_spi();
	
}

int strategy_ins::set_timeout()
{
	time_t endline1, endline2, current,span;
	current = time(NULL);
	struct tm t = *localtime(&current);
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


void strategy_ins::exit()
{
	
	if( m_strategy != NULL )
	{
		double price = 0;
		m_strategy->send( m_spi_symbol[symbol_flatten], &price, sizeof(price));
	}
	m_pause_flag = 1;
	m_strategy->quit();
	lmice_info_print("exit strategy instance\n");
	sleep(1);
	_exit();
}

void strategy_ins::_exit()
{
	m_exit_flag = 1;
}


