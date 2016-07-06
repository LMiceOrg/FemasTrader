#pragma once
#include <vector>
#include "struct_ht.h"
#include "socket_multicast.h"
#include "USTPFtdcTraderApi.h"

#include "lmice_eal_log.h"
#include "lmice_eal_bson.h"

using std::vector;

#define CUR_INS1  "rb1610"
#define CUR_INS2  "rb1610"

#define MAX_IP_LEN				32

#define QUOTE_FLAG_SUMMARY		4



#define DEMO_CACHE 1
//#define DEMO_CACHE_ONLY_TIME 1
//#define DEMO_CACHE_SYSID 1

#ifdef DEMO_CACHE_SYSID

typedef struct perf_log
{
	char test_time[1024];
	char sys_id[32];
}PERF_LOG,*PPERF_LOG;

#endif


#pragma pack(push, 1)

struct multicast_info
{
	char	m_remote_ip[MAX_IP_LEN];		///< 组播行情远端地址
	int		m_remote_port;					///< 组播行情远端端口
	char	m_local_ip[MAX_IP_LEN];			///< 组播本机地址
	int		m_local_port;					///< 组播本机端口
};

#pragma pack(pop)

class guava_quote_event
{
public:
	virtual ~guava_quote_event() {}
	/// \brief 接收到组播数据的回调事件
	//virtual void on_receive_nomal(guava_udp_head* head, guava_udp_normal* data) = 0;
	//virtual void on_receive_summary(guava_udp_head* head, guava_udp_summary* data) = 0;
	virtual void on_receive_data( pIncQuotaDataT ptrData ) = 0;
};

class guava_quote : public socket_event
{
public:
	guava_quote(void);
	~guava_quote(void);

	/// \brief 初始化
	bool init(multicast_info cffex, guava_quote_event* event);

	/// \brief 关闭
	void close();

private:
	/// \brief 组播数据接收回调接口
	virtual void on_receive_message(int id, const char* buff, unsigned int len);
	virtual void message_log(int type, void *msg);


private:
	socket_multicast		m_udp;				///< UDP行情接收接口

	multicast_info			m_cffex_info;		///< 中金接口信息
	int						m_cffex_id;			///< 中金所行情通道

	guava_quote_event*		m_ptr_event;		///< 行情回调事件接口
	EalLog                  m_logging;          /// log obj
};

