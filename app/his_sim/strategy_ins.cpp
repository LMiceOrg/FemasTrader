
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

#define FM_INTERVAL_USEC 1

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


#define MAX_LOG_SIZE 1024

///freq check, 1min 120 round
///top check, 

forceinline void do_order_insert(int volume) {
    int64_t middle_time;
    get_system_time(&middle_time);
    g_spi->order_insert(0, &g_order, sizeof(CUstpFtdcInputOrderField));
    get_system_time(&g_end_time);
    printf("=== do order insert ===\n"
           "inst time:%ld\t"
           "proc time:%ld\t"
           "direction:%c\t"
           "operation:%c\t"
           "cur price:%lf\t"
           "leftvlume:%d\t"
           "ordr size:%d\n",
          // g_end_time - ( g_pkg_time.tv_sec*10000000L+g_pkg_time.tv_usec*10L),
           g_end_time - middle_time,
           g_end_time - g_begin_time,
           g_order.Direction,
           g_order.OffsetFlag,
           g_order.LimitPrice,
           volume,
           g_order.Volume);
}

forceinline void do_order_action()
{

	if( g_order_action_count < MAX_ORDER_ACTION )
	{
		g_spi->order_action(0, &g_order_action, sizeof(CUstpFtdcOrderActionField));
		//printf(" === do order action ===\n");
	}
	else
	{
		g_ins->exit();
		lmice_critical_print("max order action,exit!!!\n");
	}
}


double deal_order( char *order_time, int order_msec )
{
	double PNL = 0;
	double money = g_order.Volume * g_cur_st->m_md.m_multiple * g_order.LimitPrice;
	double fee = money * g_cur_st->m_md.fee_rate;

	if( g_order.Direction == USTP_FTDC_D_Buy )
	{
		if( g_order.OffsetFlag == USTP_FTDC_OF_Open )
		{
			g_cur_st->m_pos.m_buy_pos += g_order.Volume; 
		}
		else
		{
			g_cur_st->m_pos.m_sell_pos -= g_order.Volume; 
		}
		PNL = -money;
		PNL -= fee;
	}
	else
	{
		if( g_order.OffsetFlag == USTP_FTDC_OF_Open )
		{
			g_cur_st->m_pos.m_sell_pos += g_order.Volume; 
		}
		else
		{
			g_cur_st->m_pos.m_buy_pos -= g_order.Volume; 
		}
		PNL = money - fee;
	}
	sprintf( log_buffer+log_pos , "%s,%s.%03d,%c,%c,%f,%d,%f,%f\n",g_order.InstrumentID, order_time, order_msec, 
																g_order.Direction, g_order.OffsetFlag, 
																g_order.LimitPrice, g_order.Volume, 
																money, fee);
	log_pos = strlen(log_buffer);
	//printf("==deal_order=%f=\n", PNL);
	session_fee += fee;
	session_trading_times++;

	return PNL;

}

//#define FLT_MIN 0.0001
#define MD_DOUBLE_EQUAL(x,y) ( ((x)-(y)) > FLT_MIN && ((x)-(y)) < FLT_MIN )

//call back function 
double md_func(const char* symbol, const void* addr, int size)
{

    guava_udp_head *ptr_head = (guava_udp_head*)addr;
	guava_udp_normal *ptr_data = NULL;
	double PNL = 0;
	static int flag = 0;

	//in case message less then flag2 condition
	if( size == 0 )
	{
		//printf("=== last flatten ===\n");
		ptr_data = (guava_udp_normal*)(addr + sizeof(guava_udp_head));
		//printf("g_cur_st->m_pos.m_sell_pos:%d\n", g_cur_st->m_pos.m_sell_pos);
		if( g_cur_st->m_pos.m_sell_pos > 0 )
		{
			g_order.Volume = g_cur_st->m_pos.m_sell_pos;
			g_order.LimitPrice = ptr_data->m_ask_px + g_cur_status.m_md.m_price_tick;
			g_order.Direction = USTP_FTDC_D_Buy;
			g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
			PNL = deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
		}
		//printf("g_cur_st->m_pos.m_buy_pos:%d\n", g_cur_st->m_pos.m_buy_pos);
		if( g_cur_st->m_pos.m_buy_pos > 0 )
		{
			g_order.Volume = g_cur_st->m_pos.m_buy_pos;
			g_order.LimitPrice = ptr_data->m_bid_px - g_cur_status.m_md.m_price_tick;
			g_order.Direction = USTP_FTDC_D_Sell;
			g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
			PNL += deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
		}
		flag = 0;
		return PNL;
	}

	if (ptr_head->m_quote_flag != QUOTE_FLAG_SUMMARY)
	{
		ptr_data = (guava_udp_normal*)(addr + sizeof(guava_udp_head));
	}
	else
	{
		return 0;
	}


    if( 1 != g_ins->get_pause_status() ) 
	{
        if( ptr_data->m_last_px != DBL_MAX &&
                ptr_data->m_bid_px!= DBL_MAX &&
                ptr_data->m_ask_px!= DBL_MAX ) 
        {   
			Forecaster *fc = g_ins->get_forecaster();
	        Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg_data;
	        msg_dt.m_inst = ptr_head->m_symbol;

			int64_t micro_time = 0;
			time_t data_time1, current;
			current = time(NULL);
			struct tm t = *localtime(&current);
			char data_time[16];
			memset(data_time, 0, 16);
			strcpy(data_time, ptr_head->m_update_time);
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
			micro_time += ptr_head->m_millisecond * 1000;
			msg_dt.m_time_micro = micro_time;
			
	        msg_dt.m_bid = ptr_data->m_bid_px;
	        msg_dt.m_offer = ptr_data->m_ask_px;
	        msg_dt.m_bid_quantity = ptr_data->m_bid_share;
	        msg_dt.m_offer_quantity = ptr_data->m_ask_share;
			
	        msg_dt.m_volume = ptr_data->m_last_share;
	        msg_dt.m_notional = ptr_data->m_total_value;
	        msg_dt.m_limit_up = 0;
	        msg_dt.m_limit_down = 0;

	        fc->update(msg_data);
	        double signal = fc->get_forecast();


			/** record signal detail to signal file - start */
			int size = 0;
			#define TMP_ARR_SIZE 128
			static double arr_signal[TMP_ARR_SIZE];
			memset( arr_signal, 0, TMP_ARR_SIZE*sizeof(double) );

			double *ptr_array = arr_signal;
			fc->get_all_signal( &ptr_array, &size);
			
			int64_t systime_fc = 0;
			get_system_time(&systime_fc);
			
			EalBson bson_text, bson_msg, bson_fc;
			bson_msg.AppendInt64("recv_local_time:", msg_data.get_time());
			bson_msg.AppendSymbol("m_recv_UpdateTime:", ptr_head->m_update_time);
			bson_msg.AppendInt32("m_recv_UpdateMillisec", ptr_head->m_millisecond);
			bson_msg.AppendSymbol("m_inst", msg_data.get_inst().c_str());
			bson_msg.AppendInt64("m_time_micro", msg_data.get_time());
			bson_msg.AppendDouble("m_cur_price", ptr_data->m_last_px);
			bson_msg.AppendDouble("m_offer", msg_data.get_offer());
			bson_msg.AppendInt32("m_offer_quantity", msg_data.get_offer_quantity());
			bson_msg.AppendDouble("m_bid", msg_data.get_bid());
			bson_msg.AppendInt32("m_bidr_quantity", msg_data.get_bid_quantity());
			bson_msg.AppendInt32("m_volume", msg_data.get_volume());
			bson_msg.AppendDouble("m_notional", msg_data.get_notional());
			bson_msg.AppendDouble("m_limit_down", msg_data.get_limit_down());
			bson_msg.AppendDouble("m_limit_up", msg_data.get_limit_up());
			
			bson_fc.AppendInt64("forecaster time", systime_fc/10);
			bson_fc.AppendDouble("final_signal", signal);
			if( 0 == strcmp( ptr_head->m_symbol, g_cur_st->m_ins_name) )
			{
				double mid = 0.5 * ( ptr_data->m_ask_px + ptr_data->m_bid_px );
				double forecast = mid * ( 1 + signal * g_signal_multiplier );
				bson_fc.AppendDouble("forecaster", forecast);
			}
			else
			{
				bson_fc.AppendDouble("forecaster", 0);
			}

			int iter;
			for( iter=0; iter<size; iter++ )
			{
				bson_fc.AppendDouble( array_feature_name[iter].c_str(), arr_signal[iter]);
			}
			
		/*	
			bson_fc.AppendDouble("ChinaL1DiscreteSelfDecayingReturnFeature_1", arr_signal[0]);
			bson_fc.AppendDouble("ChinaL1DiscreteSelfDecayingReturnFeature_2", arr_signal[1]);
			bson_fc.AppendDouble("ChinaL1DiscreteSelfBookImbalanceFeature", arr_signal[2]);
			bson_fc.AppendDouble("ChinaL1DiscreteSelfTradeImbalanceFeatureV2", arr_signal[3]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherBestSinceFeature_rb00_1", arr_signal[4]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherBestSinceFeature_rb00_2", arr_signal[5]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherDecayingReturnFeature_rb00_1", arr_signal[6]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherDecayingReturnFeature_rb00_2", arr_signal[7]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherBookImbalanceFeature_rb00", arr_signal[8]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherTradeImbalanceFeatureV2_rb00_1", arr_signal[9]);
			bson_fc.AppendDouble("ChinaL1DiscreteOtherTradeImbalanceFeatureV2_rb00_2", arr_signal[10]);
		*/


			
			bson_text.AppendDocument( "md_msg", &bson_msg );
			bson_text.AppendDocument( "fc_msg", &bson_fc );
			
			const char *strJson = bson_text.GetJsonData();
			sprintf(signal_log_buffer+signal_log_pos,"%s\n", strJson);
			signal_log_pos = strlen(signal_log_buffer);
			signal_log_line++;
			bson_text.FreeJsonData();

			/** record signal detail to signal file - end */
			
			//signal/=1.21;
	        double forecast = 0;	
			int left_pos_size = 0;

			//printf("hour:%d - min:%d\n", t.tm_hour, t.tm_min);
			if( flag == 0 )
			{
				if( (t.tm_hour == 11 && t.tm_min == 25 ) ||
					( t.tm_hour == 14 && t.tm_min == 55 ) ||
					( t.tm_hour == 22 && t.tm_min == 55 )
				  )
				{
			//		printf("=======flag 1========\n");
					flag = 1;
				}
			}
/*
			if( (t.tm_hour == 11 && t.tm_min == 24 && t.tm_sec >= 55 ) ||
				( t.tm_hour == 14 && t.tm_min >= 54 && t.tm_sec >= 55 ) ||
				( t.tm_hour == 22 && t.tm_min >= 54 && t.tm_sec >= 55 )
			  )
			{
				flag = 2;
			}*/

			if( (t.tm_hour == 11 && t.tm_min >= 28) ||
				( t.tm_hour == 14 && t.tm_min >= 58 ) ||
				( t.tm_hour == 22 && t.tm_min >= 58 )
			  )
			{
			//	printf("=======flag 1========\n");
				flag = 2;
			}
/*
			printf("-|||=== md_func msg_dt\n");
			printf("||||||=== bid: %f - volume:%d\n",msg_dt.m_bid, msg_dt.m_bid_quantity);
			printf("||||||=== ask: %f - volume:%d\n",msg_dt.m_offer, msg_dt.m_offer_quantity);
			printf("||||||=== last: %f - volume:%d\n",ptr_data->m_last_px, msg_dt.m_volume);
			printf("||||||=== value:%f - pos: %d\n", msg_dt.m_notional, ptr_data->m_total_pos); 
*/

			if( 0 == strcmp( ptr_head->m_symbol, trading_instrument) )
			{
				//pnl check,to be add


				//price check,to be add		

				double mid = 0.5 * ( ptr_data->m_bid_px + ptr_data->m_ask_px );
	            forecast = mid * ( 1 + signal * g_signal_multiplier );
				//printf("||||||=== forecaster:%f\n\n\n\n", forecast); 
/*
				printf("-|||=== md_func get instrument: %s, flag:%d\n", symbol, ptr_head->m_quote_flag);
				printf("-|||=== time: %s.%d\n", ptr_head->m_update_time, ptr_head->m_millisecond);
				printf("||||||=== bid: %f - volume:%d\n",ptr_data->m_bid_px, ptr_data->m_bid_share);
				printf("||||||=== ask: %f - volume:%d\n",ptr_data->m_ask_px, ptr_data->m_ask_share);
				printf("||||||=== signal: %f - forcaster:%f\n", signal, forecast);
				printf("||||||=== last: %f - volume:%d\n",ptr_data->m_last_px, ptr_data->m_last_share);
				printf("||||||=== value:%f - pos: %d\n\n", ptr_data->m_total_value, ptr_data->m_total_pos);	
*/				
				if( flag == 0 )
				{
					//printf("===flag 0===\n");
					if( forecast > ptr_data->m_ask_px )
					{
						//printf("=== pos === buy:%d - sell:%d\n", g_cur_st->m_pos.m_buy_pos, g_cur_st->m_pos.m_sell_pos);
						left_pos_size = g_conf->m_max_pos - ( g_cur_st->m_pos.m_buy_pos - g_cur_st->m_pos.m_sell_pos );
						if( left_pos_size <= 0 )
						{
							//printf("[BUY]left pos size <0\n");
							return 0;
						}
					
						g_order.Volume = left_pos_size;
						g_order.LimitPrice = ptr_data->m_ask_px;
						if( g_cur_st->m_pos.m_sell_pos >= left_pos_size )
						{
							g_order.Direction = USTP_FTDC_D_Buy;
							g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
						}
						else
						{
							g_order.Direction = USTP_FTDC_D_Buy;
							g_order.OffsetFlag = USTP_FTDC_OF_Open;
						}
						PNL = deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
					}
					
					if( forecast < ptr_data->m_bid_px )
					{
						//printf("=== pos === buy:%d - sell:%d\n", g_cur_st->m_pos.m_buy_pos, g_cur_st->m_pos.m_sell_pos);
						left_pos_size = g_conf->m_max_pos - ( g_cur_st->m_pos.m_sell_pos - g_cur_st->m_pos.m_buy_pos );
						if( left_pos_size <= 0 )
						{
							//printf("[Sell]left pos size <0\n");
							return 0;
						}
					
						g_order.Volume = left_pos_size;
						g_order.LimitPrice = ptr_data->m_bid_px;
					
						if( g_cur_st->m_pos.m_buy_pos >= left_pos_size )
						{
							g_order.Direction = USTP_FTDC_D_Sell;
							g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
						}
						else
						{
							g_order.Direction = USTP_FTDC_D_Sell;
							g_order.OffsetFlag = USTP_FTDC_OF_Open;
						}
						PNL = deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
					}

				}

				else if( flag == 1 )
				{
					//printf("===flag 1===\n");
					if( forecast > ptr_data->m_ask_px )
					{
						if( g_cur_st->m_pos.m_sell_pos > 0 )
						{
							g_order.Volume = g_cur_st->m_pos.m_sell_pos;
							g_order.LimitPrice = ptr_data->m_ask_px;
							g_order.Direction = USTP_FTDC_D_Buy;
							g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
							PNL = deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
						}
					}
					
					if( forecast < ptr_data->m_bid_px )
					{
						if( g_cur_st->m_pos.m_buy_pos > 0 )
						{
							g_order.Volume = g_cur_st->m_pos.m_buy_pos;
							g_order.LimitPrice = ptr_data->m_bid_px;
							g_order.Direction = USTP_FTDC_D_Sell;
							g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
							PNL = deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
						}	
					}
				}
				else
				{
					//printf("===flag 2===\n");
					//printf("g_cur_st->m_pos.m_sell_pos:%d\n", g_cur_st->m_pos.m_sell_pos);
					if( g_cur_st->m_pos.m_sell_pos > 0 )
					{
						g_order.Volume = g_cur_st->m_pos.m_sell_pos;
						g_order.LimitPrice = ptr_data->m_ask_px + g_cur_status.m_md.m_price_tick;
						g_order.Direction = USTP_FTDC_D_Buy;
						g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
						PNL = deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
					}
					//printf("g_cur_st->m_pos.m_buy_pos:%d\n", g_cur_st->m_pos.m_buy_pos);
					if( g_cur_st->m_pos.m_buy_pos > 0 )
					{
						g_order.Volume = g_cur_st->m_pos.m_buy_pos;
						g_order.LimitPrice = ptr_data->m_bid_px - g_cur_status.m_md.m_price_tick;
						g_order.Direction = USTP_FTDC_D_Sell;
						g_order.OffsetFlag = USTP_FTDC_OF_CloseToday;
						PNL += deal_order( ptr_head->m_update_time, ptr_head->m_millisecond );
					}
				}

			}
			else
			{
				//printf("\n\n\n\n");
			}
        }
    }
	//printf("==md_func PNL=%f==\n", PNL);
	return PNL;
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
	printf(" m_price_tick:%lf\n", g_cur_st->m_md.m_price_tick);

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
            lmice_critical_print("touch max loss,stop and exit!!!\n");
        }
    }

    return;
}

//class function

strategy_ins::strategy_ins(const char *model_name, STRATEGY_CONF_P ptr_conf , CLMSpi *spi):m_exit_flag(0),m_pause_flag(0)
{
    lmice_info_print("model name: %s\nmax positon: %d\nmax loss: %lf\nclose value: %lf\nprice tick:%lf\nfee rate:%lf\n",
                     g_conf->m_model_name, g_conf->m_max_pos,
                     g_conf->m_max_loss, g_conf->m_close_value,
                     g_cur_status.m_md.m_price_tick, g_cur_status.m_md.fee_rate);
    m_strategy = spi;
	m_model_name = model_name;
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
    //string type = "hc_0";
    string type = m_model_name;
    //string type = "rb_0";
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
	g_guava_quit_flag = 1;
}


