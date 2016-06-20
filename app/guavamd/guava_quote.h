#pragma once
#include <vector>
#include "socket_multicast.h"
#include "lmice_eal_log.h"
#include "lmice_eal_bson.h"

using std::vector;

#define MAX_IP_LEN				32

#define QUOTE_FLAG_SUMMARY		4


#pragma pack(push, 1)

struct guava_udp_head
{
	unsigned int	m_sequence;				///<会话编号
	char			m_exchange_id;			///<市场  0 表示中金  1表示上期
	char			m_channel_id;			///<通道编号
	char			m_symbol_type_flag;		///<合约标志
	int				m_symbol_code;			///<合约编号
	char			m_symbol[31];			///<合约
	char			m_update_time[9];		///<最后更新时间(秒)
	int				m_millisecond;			///<最后更新时间(毫秒)
	char			m_quote_flag;			///<行情标志		0 无time sale, 无lev1, 1 有time sale 无lev1, 2 无time sale, 有lev1, 3 有time sale, 有lev1, 4 summary信息
};

struct guava_udp_normal
{
	double			m_last_px;				///<最新价
	int				m_last_share;			///<最新成交量
	double			m_total_value;			///<成交金额
	double			m_total_pos;			///<持仓量
	double			m_bid_px;				///<最新买价
	int				m_bid_share;			///<最新买量
	double			m_ask_px;				///<最新卖价
	int				m_ask_share;			///<最新卖量
};

struct guava_udp_summary
{
	double			m_open;					///<今开盘
	double			m_high;					///<最高价
	double			m_low;					///<最低价
	double			m_today_close;			///<今收盘
	double			m_high_limit;			///<涨停价
	double			m_low_limit;			///<跌停价
	double			m_today_settle;			///<今结算价
	double			m_curr_delta;			///<今虚实度
};

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
	virtual void on_receive_nomal(guava_udp_head* head, guava_udp_normal* data) = 0;
	virtual void on_receive_summary(guava_udp_head* head, guava_udp_summary* data) = 0;
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

