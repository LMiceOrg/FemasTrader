#include "guava_quote.h"

#define OPT_LOG_DEBUG 1

#ifdef DEMO_CACHE
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID

extern int g_log_pos;
extern int g_id_pos;
extern PERF_LOG g_perf[25000];

#else
extern int64_t g_time[][25000];
extern int g_b_pos;
extern int g_r_pos;
extern int g_s_pos;
#endif
#else
extern char g_msg[][1024];
#endif
extern int g_pos;
#endif



guava_quote::guava_quote(void)
{
	m_cffex_id = 0;
	m_ptr_event = NULL;
}


guava_quote::~guava_quote(void)
{
}

#define PKT_SUMMARY 1
#define PKT_NORMAL 2

void guava_quote::message_log(int type, void *msg)
{
	
/*

	if(  0 == strcmp(ptr_head->m_symbol, CUR_INS)  )
	{

		int64_t systime = 0;
		get_system_time(&systime);

		
		
		EalBson bson;
		bson.AppendInt64("time", systime/10);
		
		bson.AppendInt64("m_sequence", ptr_head->m_sequence);
		bson.AppendFlag("m_exchange_id", ptr_head->m_exchange_id);
		bson.AppendFlag("m_channel_id", ptr_head->m_channel_id);
		bson.AppendFlag("m_symbol_type_flag", ptr_head->m_symbol_type_flag);
		bson.AppendInt64("m_symbol_code", ptr_head->m_symbol_code);
		bson.AppendSymbol("m_symbol", ptr_head->m_symbol);
		bson.AppendSymbol("m_update_time", ptr_head->m_update_time);
		bson.AppendInt64("m_millisecond", ptr_head->m_millisecond);
		bson.AppendFlag("m_quote_flag", ptr_head->m_quote_flag);
		const char *strJson = bson.GetJsonData();

		if( type == PKT_NORMAL )
		{
			printf("[future pkt] recv ( normal )\ncontent: %s\n", strJson);
			//m_logging.logging("[future pkt] recv ( normal ), content: %s", strJson);
		}
		else
		{
			printf("[future pkt] recv ( summary )\ncontent: %s\n", strJson);
			//m_logging.logging("[future pkt] recv ( summary ), content: %s", strJson);
		}
		bson.FreeJsonData();
	}

	*/
	
}


void guava_quote::on_receive_message(int id, const char* buff, unsigned int len)
{
	if (!m_ptr_event)
	{
		return;
	}

	if (len < (sizeof(IncQuotaDataT)))
	{
		return;
	}

	pIncQuotaDataT inc_data = (pIncQuotaDataT) buff;

	if(  (0 == strcmp(inc_data->InstTime.InstrumentID, CUR_INS1)) || 0 == strcmp(inc_data->InstTime.InstrumentID, CUR_INS2)  )
	{

		int64_t systime = 0;
		get_system_time(&systime);

		EalBson bson;
		bson.AppendInt64("RecvTime", systime/10);
		bson.AppendSymbol("InstrumentID", inc_data->InstTime.InstrumentID);
		bson.AppendSymbol("UpdateTime", inc_data->InstTime.UpdateTime);
		bson.AppendInt64("UpdateMillisec", inc_data->InstTime.UpdateMillisec);
		bson.AppendDouble("LastPrice", inc_data->PriVol.LastPrice);
		bson.AppendInt64("Volume", inc_data->PriVol.Volume);
		bson.AppendDouble("BidPrice1", inc_data->AskBidInst.BidPrice1);
		bson.AppendInt64("BidVolume1", inc_data->AskBidInst.BidVolume1);
		bson.AppendDouble("AskPrice1", inc_data->AskBidInst.AskPrice1);
		bson.AppendInt64("AskVolume1", inc_data->AskBidInst.AskVolume1);
		
		const char *strJson = bson.GetJsonData();

#ifdef DEMO_CACHE	
#ifdef DEMO_CACHE_ONLY_TIME
		
#else
		sprintf(g_msg[g_pos++],"[future pkt] recv ( normal )\ncontent: %s\n", strJson);
#endif
#else
		printf("[future pkt] recv ( normal )\ncontent: %s\n", strJson);
#endif
		bson.FreeJsonData();
		m_ptr_event->on_receive_data( (pIncQuotaDataT) buff );

	}


}


bool guava_quote::init(multicast_info cffex, guava_quote_event* p_event)
{
	m_cffex_info = cffex;
	m_ptr_event = p_event;
	
	bool ret = m_udp.sock_init(m_cffex_info.m_remote_ip, m_cffex_info.m_remote_port, m_cffex_info.m_local_ip, m_cffex_info.m_local_port, 0, this);
	
	if (!ret)
	{
		return false;
	}

	return true;
}

void guava_quote::close()
{
	m_udp.sock_close();
	m_cffex_id = 0;
}
