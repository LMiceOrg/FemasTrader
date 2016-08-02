
#include "strategy_ins.h"
#include "lmice_eal_bson.h"
#include "lmice_eal_time.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <float.h>
#include <map>
#include <vector>

#include <unistd.h>
#include <sys/types.h>

//#define FC_DEBUG 1

#include "fm2spi.h"
extern CFemas2TraderSpi * g_spi;

enum spi_symbol{
	//publish symbol
	symbol_order_insert = 0,
	symbol_order_action,
	symbol_flatten,
	symbol_hard_flatten,
	symbol_log,
	symbol_trade_ins,
	//subscribe symbol
	symbol_position,
	symbol_account
};

using namespace std;

//extern strategy_ins *g_ins;

//CLMSpi * g_spi = NULL;
//CUR_STATUS_P g_cur_st = NULL;
//STRATEGY_CONF_P g_conf = NULL;
//CUstpFtdcInputOrderField g_order;

#define MAX_LOG_SIZE 1024

///freq check, 1min 120 round
///top check, 


//call back function 
void md_func(const char* symbol, const void* addr, int size)
{
    const pIncQuotaDataT md_data = (const pIncQuotaDataT) addr;
//    lmice_critical_print("%d\t%d\t%lf\t%lf\t%lf\n",
//                         g_ins->get_pause_status(),
//                         size,
//                         md_data->AskBidInst.AskPrice1,
//                         md_data->AskBidInst.BidPrice1,
//                         md_data->PriVol.LastPrice
//                         );
    if( 1 != g_ins->get_pause_status() && size == sizeof(IncQuotaDataT) ) {
        if( md_data->AskBidInst.AskPrice1 != DBL_MAX &&
                md_data->AskBidInst.BidPrice1 != DBL_MAX &&
                md_data->PriVol.LastPrice != DBL_MAX ) {
            /*
                memset(str_log, 0, sizeof(str_log));
                sprintf( str_log, "[%ld]get md data,content: %s\n", systime_md, md_data->InstTime.InstrumentID );
                g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );
            */
            /*
                printf("\n=========get md dat:=========\n");
                printf("time\n");
                printf("ins:%s\n", md_data->InstTime.InstrumentID);
                printf("pri:%lf\n\n", md_data->PriVol.LastPrice);
            */
            #ifdef USE_C_LIB
                //translate format to FC_MD_MSG
                FC_MD_MSG md_msg;
                memset( &md_msg, 0, sizeof(FC_MD_MSG) );

                strcpy( md_msg.m_inst, md_data->InstTime.InstrumentID );

                int64_t micro_time = 0;
            
				get_system_time(&micro_time);

                md_msg.m_time_micro = micro_time/10;

                md_msg.m_bid = md_data->AskBidInst.BidPrice1;
                md_msg.m_offer =  md_data->AskBidInst.AskPrice1;
                md_msg.m_bid_quantity = md_data->AskBidInst.BidVolume1;
                md_msg.m_offer_quantity = md_data->AskBidInst.AskVolume1;
                md_msg.m_volume = md_data->PriVol.Volume;
                md_msg.m_notional = md_data->PriVol.Turnover;
                md_msg.m_limit_up = g_cur_st->m_md.m_up_price;
                md_msg.m_limit_down = g_cur_st->m_md.m_down_price;

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

            #endif

            #ifdef USE_CPLUS_LIB
                Forecaster *fc = g_ins->get_forecaster();
                static ChinaL1Msg msg_data;

                //msg_data.set_inst( md_data->InstTime.InstrumentID );

                int64_t micro_time = 0;
                get_system_time(&micro_time);
                micro_time/=10;
//                time_t data_time1, current;
//                current = time(NULL);
//                struct tm t = *localtime(&current);

//                char data_time[16];
//                memset(data_time, 0, 16);
//                strcpy(data_time, md_data->InstTime.UpdateTime);

//                char tmp[8];
//                memset(tmp, 0, 8);
//                memcpy( tmp, data_time, 2 );
//                t.tm_hour = atoi(tmp);
//                memcpy( tmp, data_time+3, 2 );
//                t.tm_min = atoi(tmp);
//                memcpy( tmp, data_time+6, 2 );
//                t.tm_sec = atoi(tmp);
//                data_time1 = mktime(&t);
//                //sscanf("%d:%d:%d", md_data->InstTime.UpdateTime, t.tm_hour, t.tm_min, t.tm_sec);
//                micro_time = (int64_t)data_time1 * 1000 * 1000;
//                micro_time += md_data->InstTime.UpdateMillisec * 1000;

                Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg_data;
                msg_dt.m_inst = md_data->InstTime.InstrumentID;
                msg_dt.m_time_micro = micro_time;
                msg_dt.m_bid = md_data->AskBidInst.BidPrice1;
                msg_dt.m_offer = md_data->AskBidInst.AskPrice1;
                msg_dt.m_bid_quantity = md_data->AskBidInst.BidVolume1;
                msg_dt.m_offer_quantity = md_data->AskBidInst.AskVolume1;
                msg_dt.m_volume = md_data->PriVol.Volume ;
                msg_dt.m_notional = md_data->PriVol.Turnover;
                msg_dt.m_limit_up = 0;
                msg_dt.m_limit_down = 0;

                fc->update(msg_data);
                double signal = fc->get_forecast();

            #ifdef FC_DEBUG
                double arr_signal[12];
                memset( arr_signal, 0, 12*sizeof(double) );
                fc->get_all_signal(arr_signal);
            #endif

            #endif

                if( 0 == strcmp( md_data->InstTime.InstrumentID, g_cur_st->m_ins_name) )
                {
                    if( md_data->PriVol.LastPrice > 0 &&
                            md_data->PriVol.LastPrice != g_cur_st->m_md.m_last_price)
                    {
                        g_cur_st->m_md.m_last_price =  md_data->PriVol.LastPrice;
                        //fix me -  modify : buy sell count, value calculate methods, fee, multiplier
                        int left_pos = g_cur_st->m_pos.m_buy_pos - g_cur_st->m_pos.m_sell_pos;
                        double fee =  fabs(left_pos) * g_cur_st->m_md.m_multiple * g_cur_st->m_md.m_last_price * g_cur_st->m_md.fee_rate;
                        double pl = g_cur_st->m_acc.m_left_cash + left_pos * g_cur_st->m_md.m_multiple * g_cur_st->m_md.m_last_price - fee;

                        if( -pl >= g_conf->m_max_loss )
                        {
                            g_ins->exit();
                            lmice_critical_print("touch max loss,stop and exit\n");
                        }

                    }

                    if( g_cur_st->m_md.m_last_price >= g_cur_st->m_md.m_up_price ||
                            g_cur_st->m_md.m_last_price <= g_cur_st->m_md.m_down_price )
                    {
                        printf("touch the limit, return\n");
                        printf("last price:%lf\n", g_cur_st->m_md.m_last_price);
                        printf("up price:%lf\n", g_cur_st->m_md.m_up_price);
                        printf("down price:%lf\n", g_cur_st->m_md.m_down_price);
                        return;
                    }

                    double mid = 0.5 * ( md_data->AskBidInst.AskPrice1 + md_data->AskBidInst.BidPrice1 );
                    double forecast = mid * ( 1 + signal );
            /*
                    int64_t systime_fc = 0;
                    get_system_time(&systime_fc);
                    printf("\n=========get forecast:=========\n");
                    printf("time:%ld\n", systime_fc/10);
                    printf("ins:%s\n", md_data->InstTime.InstrumentID);
                    printf("for:%lf\n", forecast);
                    printf("bid:%lf\n", md_data->AskBidInst.BidPrice1);
                    printf("ask:%lf\n\n", md_data->AskBidInst.AskPrice1);
            */
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
//                    lmice_debug_print("signal=%lf, forecast:%lf\tAsk:%lf\tBid:%lf\n", signal, forecast,
//                                      md_data->AskBidInst.AskPrice1,
//                                      md_data->AskBidInst.BidPrice1
//                                      );
                    //send order insert command
                    int order_size = 0;
                    int left_pos_size = 0;
                    if( forecast > md_data->AskBidInst.AskPrice1 )
                    {
                        left_pos_size = g_conf->m_max_pos - ( g_cur_st->m_pos.m_buy_pos - g_cur_st->m_pos.m_sell_pos );
                        //printf("=== left_pos_size: %d\n ===\n", left_pos_size);
                        if( left_pos_size <= 0 )
                        {
                            return;
                        }
                        order_size = left_pos_size < md_data->AskBidInst.AskVolume1 ? left_pos_size : md_data->AskBidInst.AskVolume1;
                        g_order.Volume = order_size;
                        g_order.LimitPrice = md_data->AskBidInst.AskPrice1;
                        //printf("=== order_size: %d\n ===\n", order_size);
                        if( g_cur_st->m_pos.m_sell_pos >= order_size )
                        {
                            g_order.Direction = USTP_FTDC_D_Buy;
                            g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
                        }
                        else
                        {
                            g_order.Direction = USTP_FTDC_D_Buy;
                            g_order.OffsetFlag = USTP_FTDC_OF_Open;
                        }
                        //send order insert

                        int64_t middle_time;
                        get_system_time(&middle_time);
                        g_spi->order_insert(symbol, &g_order, sizeof(CUstpFtdcInputOrderField));
                        get_system_time(&g_end_time);
                        printf("order insert time:%ld\t total time:%ld\n\noperation: buy %s\tprice:%lf\tleft_volume:%d\tleft_pos:%d\n",
							g_end_time-middle_time, g_end_time - g_begin_time,
							g_order.OffsetFlag==USTP_FTDC_OF_CloseToday?"close":"open", g_order.LimitPrice, md_data->AskBidInst.AskVolume1, left_pos_size);
                    }

                    if( forecast < md_data->AskBidInst.BidPrice1 )
                    {
                        left_pos_size = g_conf->m_max_pos - ( g_cur_st->m_pos.m_sell_pos - g_cur_st->m_pos.m_buy_pos );
                        if( left_pos_size <= 0 )
                        {
                            return;
                        }
                        order_size = left_pos_size < md_data->AskBidInst.BidVolume1 ?
                                left_pos_size : md_data->AskBidInst.BidVolume1;
                        g_order.Volume = order_size;
                        g_order.LimitPrice = md_data->AskBidInst.BidPrice1;

                        if( g_cur_st->m_pos.m_buy_pos >= order_size )
                        {
                            g_order.Direction = USTP_FTDC_D_Sell;
                            g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
                        }
                        else
                        {
                            g_order.Direction = USTP_FTDC_D_Sell;
                            g_order.OffsetFlag = USTP_FTDC_OF_Open;
                        }
                        //send order insert
                        int64_t middle_time;
                        get_system_time(&middle_time);
                        g_spi->order_insert(symbol, &g_order, sizeof(CUstpFtdcInputOrderField));
                        get_system_time(&g_end_time);
                        printf("order insert time:%ld\t total time:%ld\n\noperation: sell %s\tprice:%lf\tleft_volume:%d\tleft_pos:%d\n",
							g_end_time-middle_time, g_end_time - g_begin_time,
							g_order.OffsetFlag==USTP_FTDC_OF_CloseToday?"close":"open", g_order.LimitPrice, md_data->AskBidInst.BidVolume1, left_pos_size);
                    }


                }
        }

    }


}


void status_func(const char* symbol, const void* addr, int size)
{
	lmice_info_print("get status update\n");
	char str_log[MAX_LOG_SIZE];
	int64_t systime_md = 0;

	if( 1 == g_ins->get_pause_status() )
	{
		return;
	}

	if( size < sizeof(CUR_STATUS) )
	{
		return;
	}

	double last_price = g_cur_st->m_md.m_last_price;
	memcpy( g_cur_st, addr, sizeof(CUR_STATUS) );
	g_cur_st->m_md.m_last_price = last_price;

	printf("===current status:===\n");
	printf(" up price: %lf\n", g_cur_st->m_md.m_up_price);
	printf(" down price: %lf\n", g_cur_st->m_md.m_down_price);
	printf(" last price: %lf\n", g_cur_st->m_md.m_last_price);
	printf(" m_buy_pos:%d\n", g_cur_st->m_pos.m_buy_pos);
	printf(" m_sell_pos:%d\n", g_cur_st->m_pos.m_sell_pos);
	printf(" m_left_cash:%lf\n", g_cur_st->m_acc.m_left_cash);
	printf(" m_fee:%lf\n", g_cur_st->m_md.fee_rate);

	if( g_cur_st->m_md.m_last_price == 0 )
	{
		return;
	}

	//if( 0 == strcmp( symbol, g_ins->m_spi_symbol[symbol_account] ) )
	{
		//get_system_time(&systime_md);
		//memset(str_log, 0, sizeof(str_log));
		//sprintf( str_log, "[%ld]get trade msg,content:\n", systime_md );
        //g_spi->send( g_ins->m_spi_symbol[symbol_log], str_log, strlen(str_log) );
	}

	//calculate pl
	{
		int left_pos = g_cur_st->m_pos.m_buy_pos - g_cur_st->m_pos.m_sell_pos;
		double fee =  fabs(left_pos) * g_cur_st->m_md.m_multiple * g_cur_st->m_md.m_last_price * g_cur_st->m_md.fee_rate;
		double pl = g_cur_st->m_acc.m_left_cash + left_pos * g_cur_st->m_md.m_multiple * g_cur_st->m_md.m_last_price - fee;

		printf(" current_pl:%lf\n", pl);
		
		if( -pl >= g_conf->m_max_loss )
		{
			g_ins->exit();
			lmice_critical_print("touch max loss,stop and exit\n");
		}
	}

	return;
}

//class function

strategy_ins::strategy_ins(STRATEGY_CONF_P ptr_conf , CLMSpi *spi):m_exit_flag(0),m_pause_flag(0)
{
	lmice_info_print("model name: %s\nmax positon: %d\nmax loss: %lf\nclose value: %lf\n",
                             g_conf->m_model_name, g_conf->m_max_pos,
                             g_conf->m_max_loss, g_conf->m_close_value);
    m_strategy = spi;
    //m_status = new CUR_STATUS;
    //memset(m_status, 0, sizeof(CUR_STATUS));
    //memset( m_spi_symbol, 0, sizeof(m_spi_symbol));

    //g_spi = m_strategy;
    //g_cur_st = m_status;
    //g_conf = m_ins_conf;

	
	init();
}

strategy_ins::~strategy_ins()
{
    //delete m_status;
    //delete m_strategy;
}

void strategy_ins::init()
{	
	string type = "hc_0";
    //time_t current;
    //current = time(NULL);
    //struct tm date = *localtime(&current);
    int64_t micro_time = 0;
    get_system_time(&micro_time);
    struct tm date;
    time_t t = micro_time/1e7;
    localtime_r(&t, &date);
	
#ifdef USE_C_LIB
	if ( fc_init( "hc_0", date ) < 0 )
	{
		lmice_error_print(" forecaster lib init error\n ");
	}
#endif

#ifdef USE_CPLUS_LIB
	m_forecaster = ForecasterFactory::createForecaster( type, date );
#endif

	return;
}

void strategy_ins::run()
{

	//设置超时时间，时间到达后发出清仓指令
	if( set_timeout() < 0 )
	{
		lmice_error_print("set time out signal error\n");
		return;
	}
	
}

int strategy_ins::set_timeout()
{
	time_t endline1, endline2, endline3, current, span;
	current = time(NULL);
	struct tm t = *localtime(&current);
	current = mktime(&t);
	t.tm_hour = 11;
	t.tm_min = 25;
	t.tm_sec = 0;
	endline1 = mktime(&t);
	
	t.tm_hour = 14;
	t.tm_min = 55;
	t.tm_sec = 0;
	endline2 = mktime(&t);

	t.tm_hour = 22;
	t.tm_min = 55;
	t.tm_sec = 0;
	endline3 = mktime(&t);

	lmice_info_print( "set time out\n" );
	printf("current:%ld\n", current);
	printf("endline1:%ld\n", endline1);
	printf("endline2:%ld\n", endline2);
	printf("endline3:%ld\n", endline3);


	if( current < endline1 )
	{
		span = endline1 - current;
	}
	else if( current < endline2 )
	{
		span = endline2 - current;
	}
	else if( current < endline3 )
	{
		span = endline3 - current;
	}
	else
	{
		return -1;
	}

	printf("span:%ld\n", span);

	alarm(span);

}


void strategy_ins::exit()
{
	
	//if( m_strategy != NULL )
	//{
		//double price = 0;
		//printf("=== flatten all ===\n");
        //m_strategy->send( m_spi_symbol[symbol_flatten], &price, sizeof(price));
	//}
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


