#include "guava_quote.h"

#define OPT_LOG_DEBUG 1


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
	guava_udp_head* ptr_head = (guava_udp_head*)msg;
	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	bson.AppendInt64("time", systime);

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
		m_logging.logging("[future pkt] recv ( normal ), content: %s", strJson);
	}
	else
	{
		m_logging.logging("[future pkt] recv ( summary ), content: %s", strJson);
	}

	
	bson.FreeJsonData();
	
}


void guava_quote::on_receive_message(int id, const char* buff, unsigned int len)
{
	if (!m_ptr_event)
	{
		return;
	}

	if (len < (sizeof(guava_udp_head) + sizeof(guava_udp_normal)))
	{
		return;
	}

	guava_udp_head* ptr_head = (guava_udp_head*)buff;
	if (ptr_head->m_quote_flag != QUOTE_FLAG_SUMMARY)
	{
		guava_udp_normal* ptr_data = (guava_udp_normal*)(buff + sizeof(guava_udp_head));
#ifdef OPT_LOG_DEBUG
		message_log(PKT_NORMAL, (void *) ptr_head);
#endif 
		//m_logging.logging("[future pkt] recv md normal at(%s), symbol:%s - seq:%u", ptr_head->m_update_time, ptr_head->m_symbol, ptr_head->m_sequence, ptr_head->m_millisecond);
		m_ptr_event->on_receive_nomal(ptr_head, ptr_data);
	}
	else
	{
		guava_udp_summary* ptr_data = (guava_udp_summary*)(buff + sizeof(guava_udp_head));
#ifdef OPT_LOG_DEBUG
		message_log(PKT_SUMMARY, (void *) ptr_head);
#endif
		//m_logging.logging("[future pkt] recv md summary at(%s), symbol:%s - seq:%u", ptr_head->m_update_time, ptr_head->m_symbol, ptr_head->m_sequence, ptr_head->m_millisecond);
		m_ptr_event->on_receive_summary(ptr_head, ptr_data);
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
