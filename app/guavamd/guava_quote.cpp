#include "guava_quote.h"


guava_quote::guava_quote(void)
{
	m_cffex_id = 0;
	m_ptr_event = NULL;
}


guava_quote::~guava_quote(void)
{
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
		m_ptr_event->on_receive_nomal(ptr_head, ptr_data);
	}
	else
	{
		guava_udp_summary* ptr_data = (guava_udp_summary*)(buff + sizeof(guava_udp_head));
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
