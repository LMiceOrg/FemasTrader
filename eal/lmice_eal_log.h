#ifndef LMIC_EAL_LOG_H
#define LMIC_EAL_LOG_H

#include "lmice_udss.h"

#define SOCK_FILE "/var/run/lmiced.socket"

using namespace std;

class EalLog
{

public:
	EalLog( const char *sock_file = SOCK_FILE )
	{
		create_uds_msg((void**)&sid);
		init_uds_client(sock_file, sid);
		memset(&m_info, 0, sizeof(m_info));
    	m_info.pid = getpid();
    	m_info.tid = eal_gettid();
		m_info.type = LMICE_TRACE_TYPE;
	}

	~EalLog()
	{
		finit_uds_msg(sid);
	}


public:
	void logging( const char* format, ... );

private:
	uds_msg *sid;
	lmice_trace_info_t m_info;

};

#endif // LMIC_EAL_BSON_H


