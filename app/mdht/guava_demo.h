#pragma once
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>

#include "guava_quote.h"

#include "USTPFtdcTraderApi.h"

#include "forecaster.h"

using std::vector;


class CTraderSpiT : public CUstpFtdcTraderSpi 
{
public:
	CTraderSpiT(CUstpFtdcTraderApi *pTrader)
	{
		m_ptr_api = pTrader;
		m_MaxOrderLocalID = 1;
	}
	~CTraderSpiT()
	{

	}

	int GetLocalID()
	{
		return m_MaxOrderLocalID++;
	}

	CUstpFtdcTraderApi *GetApi()
	{
		return m_ptr_api;
	}

	void OnFrontConnected();
	void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pUserInvestor, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void OnRtnOrder( CUstpFtdcOrderField *pOrder );
	void OnRtnTrade(CUstpFtdcTradeField *pTrade); 
	int code_convert(const char *from_charset,const char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen);
	string gbktoutf8( char *pgbk);
	void OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo); 
	void OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
	CUstpFtdcTraderApi *m_ptr_api;
	int m_MaxOrderLocalID;
};


class guava_demo :public guava_quote_event
{
public:
	guava_demo(CTraderSpiT *pSpi);
	~guava_demo(void);

	/// \brief 示例函数入口函数
	void run();
	static 	void *order_thread(void *);

	FILE * m_pFileOutput;

private:

	virtual void on_receive_data( pIncQuotaDataT ptrData );
	
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
	CTraderSpiT*            m_trader_spi;
	CUstpFtdcTraderApi*     m_trader_api;
	multicast_info			m_cffex_info;		///< 中金接UDP信息
	guava_quote				m_guava;			///< 行情接收对象
	bool					m_quit_flag;		///< 退出标志
	int                    	m_flag;
	int						m_last_price;
	Forecaster*				m_forecaster;
};

