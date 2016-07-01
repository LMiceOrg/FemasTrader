#pragma once
#include <vector>
#include "guava_quote.h"
#include "lmspi.h"

using std::vector;


class guava_demo :public guava_quote_event, public CLMSpi
{
public:
	guava_demo( const char* name );
	~guava_demo(void);

	/// \brief 示例函数入口函数
	void run();

	
	FILE * m_pFileOutput;

private:
	virtual void on_receive_nomal(guava_udp_head* head, guava_udp_normal* data);
	virtual void on_receive_summary(guava_udp_head* head, guava_udp_summary* data);

	string to_string(guava_udp_head* ptr);
	string to_string(guava_udp_normal* ptr);
	string to_string(guava_udp_summary* ptr);

private:
	/// \brief 初始化参数调整方法
	void input_param();

	/// \brief 初始化
	bool init();

	/// \brief 关闭
	void close();

	/// \brief 暂停
	void pause();



private:
	multicast_info			m_cffex_info;		///< 中金接UDP信息
	guava_quote				m_guava;			///< 行情接收对象
	bool					m_quit_flag;		///< 退出标志


};

