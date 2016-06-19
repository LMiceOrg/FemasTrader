// TraderSpi.cpp: implementation of the CTraderSpi class.
//
//////////////////////////////////////////////////////////////////////

#include "fmspi.h"

#include "lmice_trace.h"
#include "lmice_eal_bson.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define OPT_LOG_DEBUG 1

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern TUstpFtdcBrokerIDType g_BrokerID;
extern TUstpFtdcUserIDType	g_UserID;

extern FILE * g_fpRecv;

int CTraderSpi::GetRequestID()
{
    static int id = 1;
    if(id < m_nOrdLocalID) {
        id = m_nOrdLocalID+1;
    }
    return ++id;
}

int CTraderSpi::loadconf(const char *name)
{
    int ret;
    const char* fname = "fmtrader.ini";
    if(name) {
        fname = name;
    }

    FILE* fp = fopen(fname, "r");
    if(!fp) {
        return -1;
    }
    memset(&m_fmconf, 0, sizeof m_fmconf);

    ret = fscanf(fp, "FrontAddr=%s\nBrokerID=%s\nUserID=%s\nPassword=%s"
                 , m_fmconf.g_FrontAddr
                 , m_fmconf.g_BrokerID
                 , m_fmconf.g_UserID
                 , m_fmconf.g_Password);
    if(ret < 4)
        ret = -1;
    else
        ret = 0;

    logging("front server: %s", m_fmconf.g_FrontAddr);
    memcpy(g_UserID, m_fmconf.g_UserID, sizeof(m_fmconf.g_UserID) );
    memcpy(g_BrokerID, m_fmconf.g_BrokerID, sizeof(m_fmconf.g_BrokerID));


    fclose(fp);
    return 0;
}

int CTraderSpi::login()
{
    CUstpFtdcReqUserLoginField req;
    memset(&req, 0, sizeof req);
    //memcpy(req.BrokerID, m_fmconf.g_BrokerID, sizeof req.BrokerID);
    strcpy(req.BrokerID, m_fmconf.g_BrokerID);
    //memcpy(req.UserID, m_fmconf.g_UserID, sizeof req.UserID);
    strcpy(req.UserID, m_fmconf.g_UserID);
    //memcpy(req.Password, m_fmconf.g_Password, sizeof req.Password);
    strcpy(req.Password, m_fmconf.g_Password);
    /*memcpy(req.UserProductInfo, m_fmconf.g_ProductInfo, sizeof req.UserProductInfo);*/
    strcpy(req.UserProductInfo, m_fmconf.g_ProductInfo);
    return m_pUserApi->ReqUserLogin(&req, GetRequestID());
}

void CTraderSpi::showtips(void) {
    int ver[2];
    const char* sver;

    sver = CUstpFtdcTraderApi::GetVersion(ver[0], ver[1]);
    lmice_critical_print("Femas Trader (%s) version:%d.%d\n\n", sver, ver[0], ver[1]);
    logging("Femas Trader (%s) version:%d.%d", sver, ver[0], ver[1]);
}

const fmconf_t *CTraderSpi::GetConf()
{
    return &m_fmconf;
}

char *CTraderSpi::GetFrontAddr()
{
    return m_fmconf.g_FrontAddr;
}

void CTraderSpi::Init()
{
    loadconf();
}

CUstpFtdcTraderApi *CTraderSpi::GetTrader()
{
    return m_pUserApi;
}


CTraderSpi::CTraderSpi(CUstpFtdcTraderApi *pTrader):m_pUserApi(pTrader)
{
    g_nOrdLocalID = 1;
    showtips();
}

CTraderSpi::~CTraderSpi()
{

}

/// 登录处理

void CTraderSpi::OnFrontConnected()
{
    logging("OnFrontConnected");
    login();
    ///printf("请求登录，BrokerID=[%s]UserID=[%s]\n",g_BrokerID,g_UserID);

}

void CTraderSpi::OnFrontDisconnected(int nReason)
{
    //链接失败
    logging("OnFrontDisconnected Reason[%d]", nReason);
}


void CTraderSpi::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
        printf("登录失败...错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str() );
        printf("ErrorID=%d\n", pRspInfo->ErrorID);
		printf("-----------------------------\n");
		return;
	}
    m_nOrdLocalID=atoi(pRspUserLogin->MaxOrderLocalID)+1;
 	printf("-----------------------------\n");
    printf("登录成功，最大本地报单号:%d\n",m_nOrdLocalID);
 	printf("-----------------------------\n");

    //用户模型
    StartAutoOrder();
}

void CTraderSpi::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
#ifdef OPT_LOG_DEBUG
	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	const char *strJson = NULL;
	bson.AppendInt64("time", systime);
#endif

	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{

#ifdef OPT_LOG_DEBUG

		bson.AppendUtf8( "reason", gbktoutf8( pRspInfo->ErrorMsg).c_str() );

		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspOrderInsert), failed , content: %s", strJson);
		
		bson.FreeJsonData();
#endif
		
		printf("-----------------------------\n");
        printf("报单失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pInputOrder==NULL)
	{
#ifdef OPT_LOG_DEBUG

		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspOrderInsert), empty , content: %s", strJson);
		
		bson.FreeJsonData();
#endif

	
		printf("没有报单数据\n");
		return;
	}

#ifdef OPT_LOG_DEBUG

	bson.AppendSymbol("BrokerID", pInputOrder->BrokerID);
	bson.AppendSymbol("ExchangeID", pInputOrder->ExchangeID);
	bson.AppendSymbol("OrderSysID", pInputOrder->OrderSysID);
	bson.AppendSymbol("InvestorID", pInputOrder->InvestorID);
	bson.AppendSymbol("UserID", pInputOrder->UserID);
	bson.AppendSymbol("InstrumentID", pInputOrder->InstrumentID);
	bson.AppendSymbol("UserOrderLocalID", pInputOrder->UserOrderLocalID);
	bson.AppendFlag("OrderPriceType", pInputOrder->OrderPriceType);
	bson.AppendFlag("Direction", pInputOrder->Direction);
	bson.AppendFlag("OffsetFlag", pInputOrder->OffsetFlag);
	bson.AppendFlag("HedgeFlag", pInputOrder->HedgeFlag);
	bson.AppendDouble("LimitPrice",pInputOrder->LimitPrice);
	bson.AppendInt64("Volume",pInputOrder->Volume);
	bson.AppendFlag("TimeCondition", pInputOrder->TimeCondition);
	bson.AppendSymbol("GTDDate", pInputOrder->GTDDate);
	bson.AppendFlag("VolumeCondition", pInputOrder->VolumeCondition);
	bson.AppendInt64("MinVolume",pInputOrder->MinVolume);
	bson.AppendDouble("StopPrice",pInputOrder->StopPrice);
	bson.AppendFlag("ForceCloseReason", pInputOrder->ForceCloseReason);
	bson.AppendInt64("MinVolume",pInputOrder->MinVolume);
	bson.AppendInt64("IsAutoSuspend",pInputOrder->IsAutoSuspend);
	bson.AppendSymbol("BusinessUnit", pInputOrder->BusinessUnit);
	bson.AppendSymbol("UserCustom", pInputOrder->UserCustom);
	bson.AppendInt64("BusinessLocalID",pInputOrder->BusinessLocalID);
	bson.AppendSymbol("ActionDay", pInputOrder->ActionDay);


	strJson = bson.GetJsonData();

	logging("[future opt] recv (OnRspOrderInsert), success , content: %s", strJson);

	bson.FreeJsonData();
#endif

	
	printf("-----------------------------\n");
	printf("报单成功\n");
	printf("-----------------------------\n");
	return ;
	
}


void CTraderSpi::OnRtnTrade(CUstpFtdcTradeField *pTrade)
{

#ifdef OPT_LOG_DEBUG
	
		int64_t systime = 0;
		get_system_time(&systime);
		EalBson bson;
		bson.AppendInt64("time", systime);

		bson.AppendSymbol("BrokerID", pTrade->BrokerID);
		bson.AppendSymbol("ExchangeID", pTrade->ExchangeID);
		bson.AppendSymbol("TradingDay", pTrade->TradingDay);
		bson.AppendSymbol("ParticipantID", pTrade->ParticipantID);
		bson.AppendSymbol("InvestorID", pTrade->InvestorID);
		bson.AppendSymbol("ClientID", pTrade->ClientID);
		bson.AppendSymbol("SeatID", pTrade->SeatID);
		bson.AppendSymbol("UserID", pTrade->UserID);
		bson.AppendSymbol("TradeID", pTrade->TradeID);
		bson.AppendSymbol("OrderSysID", pTrade->OrderSysID);
		bson.AppendSymbol("UserOrderLocalID", pTrade->UserOrderLocalID);
		bson.AppendSymbol("InstrumentID", pTrade->InstrumentID);
		bson.AppendFlag("Direction", pTrade->Direction);
		bson.AppendFlag("OffsetFlag", pTrade->OffsetFlag);
		bson.AppendFlag("HedgeFlag", pTrade->HedgeFlag);
		bson.AppendDouble("TradePrice", pTrade->TradePrice);
		bson.AppendInt64("TradeVolume", pTrade->TradeVolume);
		bson.AppendSymbol("TradeTime", pTrade->TradeTime);			
		bson.AppendSymbol("ClearingPartID", pTrade->ClearingPartID);	
		bson.AppendInt64("BusinessLocalID", pTrade->BusinessLocalID);
		bson.AppendSymbol("ActionDay", pTrade->ActionDay);	
	
		const char *strJson = bson.GetJsonData();
	
		logging("[future opt] recv ( OnRtnTrade ), content: %s", strJson);
	
		bson.FreeJsonData();
#endif 

	printf("-----------------------------\n");
	printf("收到成交回报\n");
	Show(pTrade);
	printf("-----------------------------\n");
	return;
}

void CTraderSpi::Show(CUstpFtdcOrderField *pOrder)
{
	printf("-----------------------------\n");
	printf("交易所代码=[%s]\n",pOrder->ExchangeID);
	printf("交易日=[%s]\n",pOrder->TradingDay);
	printf("会员编号=[%s]\n",pOrder->ParticipantID);
	printf("下单席位号=[%s]\n",pOrder->SeatID);
	printf("投资者编号=[%s]\n",pOrder->InvestorID);
	printf("客户号=[%s]\n",pOrder->ClientID);
	printf("系统报单编号=[%s]\n",pOrder->OrderSysID);
	printf("本地报单编号=[%s]\n",pOrder->OrderLocalID);
	printf("用户本地报单号=[%s]\n",pOrder->UserOrderLocalID);
	printf("合约代码=[%s]\n",pOrder->InstrumentID);
	printf("报单价格条件=[%c]\n",pOrder->OrderPriceType);
	printf("买卖方向=[%c]\n",pOrder->Direction);
	printf("开平标志=[%c]\n",pOrder->OffsetFlag);
	printf("投机套保标志=[%c]\n",pOrder->HedgeFlag);
	printf("价格=[%lf]\n",pOrder->LimitPrice);
	printf("数量=[%d]\n",pOrder->Volume);
	printf("报单来源=[%c]\n",pOrder->OrderSource);
	printf("报单状态=[%c]\n",pOrder->OrderStatus);
	printf("报单时间=[%s]\n",pOrder->InsertTime);
	printf("撤销时间=[%s]\n",pOrder->CancelTime);
	printf("有效期类型=[%c]\n",pOrder->TimeCondition);
	printf("GTD日期=[%s]\n",pOrder->GTDDate);
	printf("最小成交量=[%d]\n",pOrder->MinVolume);
	printf("止损价=[%lf]\n",pOrder->StopPrice);
	printf("强平原因=[%c]\n",pOrder->ForceCloseReason);
	printf("自动挂起标志=[%d]\n",pOrder->IsAutoSuspend);
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnRtnOrder(CUstpFtdcOrderField *pOrder)
{

#ifdef OPT_LOG_DEBUG
		
			int64_t systime = 0;
			get_system_time(&systime);
			EalBson bson;
			bson.AppendInt64("time", systime);

			bson.AppendSymbol("BrokerID", pOrder->BrokerID);
			bson.AppendSymbol("ExchangeID", pOrder->ExchangeID);
			bson.AppendSymbol("OrderSysID", pOrder->OrderSysID);
			bson.AppendSymbol("InvestorID", pOrder->InvestorID);
			bson.AppendSymbol("UserID", pOrder->UserID);
			bson.AppendSymbol("InstrumentID", pOrder->InstrumentID);
			bson.AppendSymbol("UserOrderLocalID", pOrder->UserOrderLocalID);
			bson.AppendFlag("OrderPriceType", pOrder->OrderPriceType);
			bson.AppendFlag("Direction", pOrder->Direction);
			bson.AppendFlag("OffsetFlag", pOrder->OffsetFlag);
			bson.AppendFlag("HedgeFlag", pOrder->HedgeFlag);
			bson.AppendDouble("LimitPrice", pOrder->LimitPrice);
			bson.AppendInt64("Volume", pOrder->Volume);
			bson.AppendFlag("TimeCondition", pOrder->TimeCondition);
			bson.AppendSymbol("GTDDate", pOrder->GTDDate);
			bson.AppendFlag("VolumeCondition", pOrder->VolumeCondition);
			bson.AppendInt64("MinVolume", pOrder->MinVolume);
			bson.AppendDouble("StopPrice", pOrder->StopPrice);
			bson.AppendFlag("ForceCloseReason", pOrder->ForceCloseReason);
			bson.AppendInt64("IsAutoSuspend", pOrder->IsAutoSuspend);
			bson.AppendSymbol("BusinessUnit", pOrder->BusinessUnit);
			bson.AppendSymbol("UserCustom", pOrder->UserCustom);
			bson.AppendInt64("BusinessLocalID", pOrder->BusinessLocalID);
			bson.AppendSymbol("ActionDay", pOrder->ActionDay);
			bson.AppendSymbol("TradingDay", pOrder->TradingDay);
			bson.AppendSymbol("ParticipantID", pOrder->ParticipantID);
			bson.AppendSymbol("ClientID", pOrder->ClientID);
			bson.AppendSymbol("SeatID", pOrder->SeatID);
			bson.AppendSymbol("InsertTime", pOrder->InsertTime);
			bson.AppendSymbol("OrderLocalID", pOrder->OrderLocalID);
			bson.AppendFlag("OrderSource", pOrder->OrderSource);
			bson.AppendFlag("OrderStatus", pOrder->OrderStatus);
			bson.AppendSymbol("CancelTime", pOrder->CancelTime);
			bson.AppendSymbol("CancelUserID", pOrder->CancelUserID);
			bson.AppendInt64("VolumeTraded", pOrder->VolumeTraded);
			bson.AppendInt64("VolumeRemain", pOrder->VolumeRemain);

			const char *strJson = bson.GetJsonData();
		
			logging("[future opt] recv ( OnRtnOrder ), content: %s", strJson);
		
			bson.FreeJsonData();
#endif 


	printf("-----------------------------\n");
	printf("收到报单回报\n");
	Show(pOrder);
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

#ifdef OPT_LOG_DEBUG
		int64_t systime = 0;
		get_system_time(&systime);
		EalBson bson;
		const char *strJson = NULL;
		bson.AppendInt64("time", systime);
#endif


	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{

#ifdef OPT_LOG_DEBUG
		
		bson.AppendUtf8( "reason", gbktoutf8( pRspInfo->ErrorMsg).c_str() );

		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspOrderAction), failed , content: %s", strJson);
		
		bson.FreeJsonData();
#endif

		printf("-----------------------------\n");
        printf("撤单失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pOrderAction==NULL)
	{
#ifdef OPT_LOG_DEBUG
		
		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspOrderAction), empty , content: %s", strJson);
		
		bson.FreeJsonData();
#endif

		printf("没有撤单数据\n");
		return;
	}

#ifdef OPT_LOG_DEBUG
	
	bson.AppendSymbol("BrokerID", pOrderAction->BrokerID);
	bson.AppendSymbol("ExchangeID", pOrderAction->ExchangeID);
	bson.AppendSymbol("OrderSysID", pOrderAction->OrderSysID);
	bson.AppendSymbol("InvestorID", pOrderAction->InvestorID);
	bson.AppendSymbol("UserID", pOrderAction->UserID);
	bson.AppendSymbol("UserOrderLocalID", pOrderAction->UserOrderLocalID);
	bson.AppendSymbol("UserOrderActionLocalID", pOrderAction->UserOrderActionLocalID);
	bson.AppendFlag("ActionFlag", pOrderAction->ActionFlag);
	bson.AppendDouble("LimitPrice",pOrderAction->LimitPrice);
	bson.AppendInt64("VolumeChange",pOrderAction->VolumeChange);
	bson.AppendInt64("BusinessLocalID",pOrderAction->BusinessLocalID);

	strJson = bson.GetJsonData();

	logging("[future opt] recv (OnRspOrderAction), success , content: %s", strJson);

	bson.FreeJsonData();
#endif

	printf("-----------------------------\n");
	printf("撤单成功\n");
	printf("-----------------------------\n");
	return ;
}
void CTraderSpi::OnRspUserPasswordUpdate(CUstpFtdcUserPasswordUpdateField *pUserPasswordUpdate, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
        printf("修改密码失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pUserPasswordUpdate==NULL)
	{
		printf("没有修改密码数据\n");
		return;
	}
	printf("-----------------------------\n");
	printf("修改密码成功\n");
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo)
{

#ifdef OPT_LOG_DEBUG
	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	const char *strJson = NULL;
	bson.AppendInt64("time", systime);
#endif


	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{

#ifdef OPT_LOG_DEBUG
		
		bson.AppendUtf8( "reason", gbktoutf8( pRspInfo->ErrorMsg).c_str() );

		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnErrRtnOrderInsert), failed , content: %s", strJson);
		
		bson.FreeJsonData();
#endif
		printf("-----------------------------\n");
        printf("报单错误回报失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pInputOrder==NULL)
	{
#ifdef OPT_LOG_DEBUG
				
		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnErrRtnOrderInsert), empty , content: %s", strJson);
		
		bson.FreeJsonData();
#endif

	
		printf("没有数据\n");
		return;
	}

#ifdef OPT_LOG_DEBUG
	
		bson.AppendSymbol("BrokerID", pInputOrder->BrokerID);
		bson.AppendSymbol("ExchangeID", pInputOrder->ExchangeID);
		bson.AppendSymbol("OrderSysID", pInputOrder->OrderSysID);
		bson.AppendSymbol("InvestorID", pInputOrder->InvestorID);
		bson.AppendSymbol("UserID", pInputOrder->UserID);
		bson.AppendSymbol("InstrumentID", pInputOrder->InstrumentID);
		bson.AppendSymbol("UserOrderLocalID", pInputOrder->UserOrderLocalID);
		bson.AppendFlag("OrderPriceType", pInputOrder->OrderPriceType);
		bson.AppendFlag("Direction", pInputOrder->Direction);
		bson.AppendFlag("OffsetFlag", pInputOrder->OffsetFlag);
		bson.AppendFlag("HedgeFlag", pInputOrder->HedgeFlag);
		bson.AppendDouble("LimitPrice",pInputOrder->LimitPrice);
		bson.AppendInt64("Volume",pInputOrder->Volume);
		bson.AppendFlag("TimeCondition", pInputOrder->TimeCondition);
		bson.AppendSymbol("GTDDate", pInputOrder->GTDDate);
		bson.AppendFlag("VolumeCondition", pInputOrder->VolumeCondition);
		bson.AppendInt64("MinVolume",pInputOrder->MinVolume);
		bson.AppendDouble("StopPrice",pInputOrder->StopPrice);
		bson.AppendFlag("ForceCloseReason", pInputOrder->ForceCloseReason);
		bson.AppendInt64("MinVolume",pInputOrder->MinVolume);
		bson.AppendInt64("IsAutoSuspend",pInputOrder->IsAutoSuspend);
		bson.AppendSymbol("BusinessUnit", pInputOrder->BusinessUnit);
		bson.AppendSymbol("UserCustom", pInputOrder->UserCustom);
		bson.AppendInt64("BusinessLocalID",pInputOrder->BusinessLocalID);
		bson.AppendSymbol("ActionDay", pInputOrder->ActionDay);
	
	
		strJson = bson.GetJsonData();
	
		logging("[future opt] recv (OnErrRtnOrderInsert), success , content: %s", strJson);
	
		bson.FreeJsonData();
#endif


	
	printf("-----------------------------\n");
	printf("报单错误回报\n");
	printf("-----------------------------\n");
	return ;
}
void CTraderSpi::OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo)
{

#ifdef OPT_LOG_DEBUG
		int64_t systime = 0;
		get_system_time(&systime);
		EalBson bson;
		const char *strJson = NULL;
		bson.AppendInt64("time", systime);
#endif

	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
	
#ifdef OPT_LOG_DEBUG
				
				bson.AppendUtf8( "reason", gbktoutf8( pRspInfo->ErrorMsg).c_str() );
		
				strJson = bson.GetJsonData();
				
				logging("[future opt] recv (OnErrRtnOrderAction), failed , content: %s", strJson);
				
				bson.FreeJsonData();
#endif

	
		printf("-----------------------------\n");
        printf("撤单错误回报失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pOrderAction==NULL)
	{
#ifdef OPT_LOG_DEBUG
						
				strJson = bson.GetJsonData();
				
				logging("[future opt] recv (OnErrRtnOrderAction), empty , content: %s", strJson);
				
				bson.FreeJsonData();
#endif

	
		printf("没有数据\n");
		return;
	}


#ifdef OPT_LOG_DEBUG
		
		bson.AppendSymbol("BrokerID", pOrderAction->BrokerID);
		bson.AppendSymbol("ExchangeID", pOrderAction->ExchangeID);
		bson.AppendSymbol("OrderSysID", pOrderAction->OrderSysID);
		bson.AppendSymbol("InvestorID", pOrderAction->InvestorID);
		bson.AppendSymbol("UserID", pOrderAction->UserID);
		bson.AppendSymbol("UserOrderLocalID", pOrderAction->UserOrderLocalID);
		bson.AppendSymbol("UserOrderActionLocalID", pOrderAction->UserOrderActionLocalID);
		bson.AppendFlag("ActionFlag", pOrderAction->ActionFlag);
		bson.AppendDouble("LimitPrice",pOrderAction->LimitPrice);
		bson.AppendInt64("VolumeChange",pOrderAction->VolumeChange);
		bson.AppendInt64("BusinessLocalID",pOrderAction->BusinessLocalID);
	
		strJson = bson.GetJsonData();
	
		logging("[future opt] recv (OnErrRtnOrderAction), success , content: %s", strJson);
	
		bson.FreeJsonData();
#endif

	printf("-----------------------------\n");
	printf("撤单错误回报\n");
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

#ifdef OPT_LOG_DEBUG
			int64_t systime = 0;
			get_system_time(&systime);
			EalBson bson;
			const char *strJson = NULL;
			bson.AppendInt64("time", systime);
#endif

	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{

#ifdef OPT_LOG_DEBUG
				
		bson.AppendUtf8( "reason", gbktoutf8( pRspInfo->ErrorMsg).c_str() );

		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspQryOrder), failed , content: %s", strJson);
		
		bson.FreeJsonData();
#endif
		printf("-----------------------------\n");
        printf("查询报单失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pOrder==NULL)
	{
#ifdef OPT_LOG_DEBUG
						
		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspQryOrder), empty , content: %s", strJson);
		
		bson.FreeJsonData();
#endif

		printf("没有查询到报单数据\n");
		return;
	}

#ifdef OPT_LOG_DEBUG
			
	bson.AppendSymbol("BrokerID", pOrder->BrokerID);
	bson.AppendSymbol("ExchangeID", pOrder->ExchangeID);
	bson.AppendSymbol("OrderSysID", pOrder->OrderSysID);
	bson.AppendSymbol("InvestorID", pOrder->InvestorID);
	bson.AppendSymbol("UserID", pOrder->UserID);
	bson.AppendSymbol("InstrumentID", pOrder->InstrumentID);
	bson.AppendSymbol("UserOrderLocalID", pOrder->UserOrderLocalID);
	bson.AppendFlag("OrderPriceType", pOrder->OrderPriceType);
	bson.AppendFlag("Direction", pOrder->Direction);
	bson.AppendFlag("OffsetFlag", pOrder->OffsetFlag);
	bson.AppendFlag("HedgeFlag", pOrder->HedgeFlag);
	bson.AppendDouble("LimitPrice", pOrder->LimitPrice);
	bson.AppendInt64("Volume", pOrder->Volume);
	bson.AppendFlag("TimeCondition", pOrder->TimeCondition);
	bson.AppendSymbol("GTDDate", pOrder->GTDDate);
	bson.AppendFlag("VolumeCondition", pOrder->VolumeCondition);
	bson.AppendInt64("MinVolume", pOrder->MinVolume);
	bson.AppendDouble("StopPrice", pOrder->StopPrice);
	bson.AppendFlag("ForceCloseReason", pOrder->ForceCloseReason);
	bson.AppendInt64("IsAutoSuspend", pOrder->IsAutoSuspend);
	bson.AppendSymbol("BusinessUnit", pOrder->BusinessUnit);
	bson.AppendSymbol("UserCustom", pOrder->UserCustom);
	bson.AppendInt64("BusinessLocalID", pOrder->BusinessLocalID);
	bson.AppendSymbol("ActionDay", pOrder->ActionDay);
	bson.AppendSymbol("TradingDay", pOrder->TradingDay);
	bson.AppendSymbol("ParticipantID", pOrder->ParticipantID);
	bson.AppendSymbol("ClientID", pOrder->ClientID);
	bson.AppendSymbol("SeatID", pOrder->SeatID);
	bson.AppendSymbol("InsertTime", pOrder->InsertTime);
	bson.AppendSymbol("OrderLocalID", pOrder->OrderLocalID);
	bson.AppendFlag("OrderSource", pOrder->OrderSource);
	bson.AppendFlag("OrderStatus", pOrder->OrderStatus);
	bson.AppendSymbol("CancelTime", pOrder->CancelTime);
	bson.AppendSymbol("CancelUserID", pOrder->CancelUserID);
	bson.AppendInt64("VolumeTraded", pOrder->VolumeTraded);
	bson.AppendInt64("VolumeRemain", pOrder->VolumeRemain);


	strJson = bson.GetJsonData();

	logging("[future opt] recv (OnRspQryOrder), success , content: %s", strJson);

	bson.FreeJsonData();
#endif

	Show(pOrder);
	return ;
}
void CTraderSpi::Show(CUstpFtdcTradeField *pTrade)
{
	printf("-----------------------------\n");
	printf("交易所代码=[%s]\n",pTrade->ExchangeID);
	printf("交易日=[%s]\n",pTrade->TradingDay);
	printf("会员编号=[%s]\n",pTrade->ParticipantID);
	printf("下单席位号=[%s]\n",pTrade->SeatID);
	printf("投资者编号=[%s]\n",pTrade->InvestorID);
	printf("客户号=[%s]\n",pTrade->ClientID);
	printf("成交编号=[%s]\n",pTrade->TradeID);

	printf("用户本地报单号=[%s]\n",pTrade->UserOrderLocalID);
	printf("合约代码=[%s]\n",pTrade->InstrumentID);
	printf("买卖方向=[%c]\n",pTrade->Direction);
	printf("开平标志=[%c]\n",pTrade->OffsetFlag);
	printf("投机套保标志=[%c]\n",pTrade->HedgeFlag);
	printf("成交价格=[%lf]\n",pTrade->TradePrice);
	printf("成交数量=[%d]\n",pTrade->TradeVolume);
	printf("清算会员编号=[%s]\n",pTrade->ClearingPartID);
	
	printf("-----------------------------\n");
	return ;
}
void CTraderSpi::OnRspQryTrade(CUstpFtdcTradeField *pTrade, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

#ifdef OPT_LOG_DEBUG
				int64_t systime = 0;
				get_system_time(&systime);
				EalBson bson;
				const char *strJson = NULL;
				bson.AppendInt64("time", systime);
#endif

	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
#ifdef OPT_LOG_DEBUG
						
				bson.AppendUtf8( "reason", gbktoutf8( pRspInfo->ErrorMsg).c_str() );
		
				strJson = bson.GetJsonData();
				
				logging("[future opt] recv (OnRspQryTrade), failed , content: %s", strJson);
				
				bson.FreeJsonData();
#endif

	
		printf("-----------------------------\n");
        printf("查询成交失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if(pTrade==NULL)
	{
#ifdef OPT_LOG_DEBUG
						
		strJson = bson.GetJsonData();
		
		logging("[future opt] recv (OnRspQryTrade), empty , content: %s", strJson);
		
		bson.FreeJsonData();
#endif
		printf("没有查询到成交数据");
		return;
	}

#ifdef OPT_LOG_DEBUG
				
	bson.AppendSymbol("BrokerID", pTrade->BrokerID);
	bson.AppendSymbol("ExchangeID", pTrade->ExchangeID);
	bson.AppendSymbol("TradingDay", pTrade->TradingDay);
	bson.AppendSymbol("ParticipantID", pTrade->ParticipantID);
	bson.AppendSymbol("InvestorID", pTrade->InvestorID);
	bson.AppendSymbol("ClientID", pTrade->ClientID);
	bson.AppendSymbol("SeatID", pTrade->SeatID);
	bson.AppendSymbol("UserID", pTrade->UserID);
	bson.AppendSymbol("TradeID", pTrade->TradeID);
	bson.AppendSymbol("OrderSysID", pTrade->OrderSysID);
	bson.AppendSymbol("UserOrderLocalID", pTrade->UserOrderLocalID);
	bson.AppendSymbol("InstrumentID", pTrade->InstrumentID);
	bson.AppendFlag("Direction", pTrade->Direction);
	bson.AppendFlag("OffsetFlag", pTrade->OffsetFlag);
	bson.AppendFlag("HedgeFlag", pTrade->HedgeFlag);
	bson.AppendDouble("TradePrice", pTrade->TradePrice);
	bson.AppendInt64("TradeVolume", pTrade->TradeVolume);
	bson.AppendSymbol("TradeTime", pTrade->TradeTime);			
	bson.AppendSymbol("ClearingPartID", pTrade->ClearingPartID);	
	bson.AppendInt64("BusinessLocalID", pTrade->BusinessLocalID);
	bson.AppendSymbol("ActionDay", pTrade->ActionDay);	



	strJson = bson.GetJsonData();

	logging("[future opt] recv (OnRspQryOrder), success , content: %s", strJson);

	bson.FreeJsonData();
#endif

	
	Show(pTrade);
	return ;
}
void CTraderSpi::OnRspQryExchange(CUstpFtdcRspExchangeField *pRspExchange, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
        printf("查询交易所失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if (pRspExchange==NULL)
	{
		printf("没有查询到交易所信息\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("[%s]\n",pRspExchange->ExchangeID);
	printf("[%s]\n",pRspExchange->ExchangeName);
	printf("-----------------------------\n");
	return;
}

void CTraderSpi::OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
        printf("查询投资者账户失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	
	if (pRspInvestorAccount==NULL)
	{
		printf("没有查询到投资者账户\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("投资者编号=[%s]\n",pRspInvestorAccount->InvestorID);
	printf("资金帐号=[%s]\n",pRspInvestorAccount->AccountID);
	printf("上次结算准备金=[%lf]\n",pRspInvestorAccount->PreBalance);
	printf("入金金额=[%lf]\n",pRspInvestorAccount->Deposit);
	printf("出金金额=[%lf]\n",pRspInvestorAccount->Withdraw);
	printf("冻结的保证金=[%lf]\n",pRspInvestorAccount->FrozenMargin);
	printf("冻结手续费=[%lf]\n",pRspInvestorAccount->FrozenFee);
	printf("手续费=[%lf]\n",pRspInvestorAccount->Fee);
	printf("平仓盈亏=[%lf]\n",pRspInvestorAccount->CloseProfit);
	printf("持仓盈亏=[%lf]\n",pRspInvestorAccount->PositionProfit);
	printf("可用资金=[%lf]\n",pRspInvestorAccount->Available);
	printf("多头冻结的保证金=[%lf]\n",pRspInvestorAccount->LongFrozenMargin);
	printf("空头冻结的保证金=[%lf]\n",pRspInvestorAccount->ShortFrozenMargin);
	printf("多头保证金=[%lf]\n",pRspInvestorAccount->LongMargin);
	printf("空头保证金=[%lf]\n",pRspInvestorAccount->ShortMargin);
	printf("-----------------------------\n");

}

void CTraderSpi::OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pUserInvestor, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
        printf("查询可用投资者失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		printf("-----------------------------\n");
		return;
	}
	if (pUserInvestor==NULL)
	{
		printf("No data\n");
		return ;
	}
	
	printf("InvestorID=[%s]\n",pUserInvestor->InvestorID);
}

void CTraderSpi::Show(CUstpFtdcRspInstrumentField *pRspInstrument)
{
//	printf("-----------------------------\n");
//	printf("交易所代码=[%s]\n",pRspInstrument->ExchangeID);
	printf("品种代码=[%s]\n",pRspInstrument->ProductID);
	printf("品种名称=[%s]\n",pRspInstrument->ProductName);
//	printf("合约代码=[%s]\n",pRspInstrument->InstrumentID);
//	printf("合约名称=[%s]\n",pRspInstrument->InstrumentName);
//	printf("交割年份=[%d]\n",pRspInstrument->DeliveryYear);
//	printf("交割月=[%d]\n",pRspInstrument->DeliveryMonth);
//	printf("限价单最大下单量=[%d]\n",pRspInstrument->MaxLimitOrderVolume);
//	printf("限价单最小下单量=[%d]\n",pRspInstrument->MinLimitOrderVolume);
//	printf("市价单最大下单量=[%d]\n",pRspInstrument->MaxMarketOrderVolume);
//	printf("市价单最小下单量=[%d]\n",pRspInstrument->MinMarketOrderVolume);
	
//	printf("数量乘数=[%d]\n",pRspInstrument->VolumeMultiple);
//	printf("报价单位=[%lf]\n",pRspInstrument->PriceTick);
//	printf("币种=[%c]\n",pRspInstrument->Currency);
//	printf("多头限仓=[%d]\n",pRspInstrument->LongPosLimit);
//	printf("空头限仓=[%d]\n",pRspInstrument->ShortPosLimit);
//	printf("跌停板价=[%lf]\n",pRspInstrument->LowerLimitPrice);
//	printf("涨停板价=[%lf]\n",pRspInstrument->UpperLimitPrice);
//	printf("昨结算=[%lf]\n",pRspInstrument->PreSettlementPrice);
//	printf("合约交易状态=[%c]\n",pRspInstrument->InstrumentStatus);
	
//	printf("创建日=[%s]\n",pRspInstrument->CreateDate);
//	printf("上市日=[%s]\n",pRspInstrument->OpenDate);
//	printf("到期日=[%s]\n",pRspInstrument->ExpireDate);
//	printf("开始交割日=[%s]\n",pRspInstrument->StartDelivDate);
//	printf("最后交割日=[%s]\n",pRspInstrument->EndDelivDate);
//	printf("挂牌基准价=[%lf]\n",pRspInstrument->BasisPrice);
//	printf("当前是否交易=[%d]\n",pRspInstrument->IsTrading);
//	printf("基础商品代码=[%s]\n",pRspInstrument->UnderlyingInstrID);
//	printf("持仓类型=[%c]\n",pRspInstrument->PositionType);
//	printf("执行价=[%lf]\n",pRspInstrument->StrikePrice);
//	printf("期权类型=[%c]\n",pRspInstrument->OptionsType);
	printf("-----------------------------\n");
	
}
void CTraderSpi::OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
        printf("查询交易编码失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		return;
	}
	
	if (pRspInstrument==NULL)
	{
		printf("没有查询到合约数据\n");
		return ;
	}
	
	Show(pRspInstrument);
	return ;
}

void CTraderSpi::OnRspQryTradingCode(CUstpFtdcRspTradingCodeField *pTradingCode, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
        printf("查询交易编码失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		return;
	}
	
	if (pTradingCode==NULL)
	{
		printf("没有查询到交易编码\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("交易所代码=[%s]\n",pTradingCode->ExchangeID);
	printf("经纪公司编号=[%s]\n",pTradingCode->BrokerID);
	printf("投资者编号=[%s]\n",pTradingCode->InvestorID);
	printf("客户代码=[%s]\n",pTradingCode->ClientID);
	printf("客户代码权限=[%d]\n",pTradingCode->ClientRight);
	printf("是否活跃=[%c]\n",pTradingCode->IsActive);
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
        printf("查询投资者持仓 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		return;
	}
	
	if (pRspInvestorPosition==NULL)
	{
		printf("没有查询到投资者持仓\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("交易所代码=[%s]\n",pRspInvestorPosition->ExchangeID);
	printf("经纪公司编号=[%s]\n",pRspInvestorPosition->BrokerID);
	printf("投资者编号=[%s]\n",pRspInvestorPosition->InvestorID);
	printf("客户代码=[%s]\n",pRspInvestorPosition->ClientID);
	printf("合约代码=[%s]\n",pRspInvestorPosition->InstrumentID);
	printf("买卖方向=[%c]\n",pRspInvestorPosition->Direction);
	printf("今持仓量=[%d]\n",pRspInvestorPosition->Position);
	printf("-----------------------------\n");
	return ;

}

	///投资者手续费率查询应答
void CTraderSpi::OnRspQryInvestorFee(CUstpFtdcInvestorFeeField *pInvestorFee, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
        printf("查询投资者手续费率失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		return;
	}
	
	if (pInvestorFee==NULL)
	{
		printf("没有查询到投资者手续费率\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("经纪公司编号=[%s]\n",pInvestorFee->BrokerID);
	printf("合约代码=[%s]\n",pInvestorFee->InstrumentID);
	printf("客户代码=[%s]\n",pInvestorFee->ClientID);
	printf("开仓手续费按比例=[%f]\n",pInvestorFee->OpenFeeRate);
	printf("开仓手续费按手数=[%f]\n",pInvestorFee->OpenFeeAmt);
	printf("平仓手续费按比例=[%f]\n",pInvestorFee->OffsetFeeRate);
	printf("平仓手续费按手数=[%f]\n",pInvestorFee->OffsetFeeAmt);
	printf("平今仓手续费按比例=[%f]\n",pInvestorFee->OTFeeRate);
	printf("平今仓手续费按手数=[%f]\n",pInvestorFee->OTFeeAmt);
	printf("-----------------------------\n");
	return ;

}

	///投资者保证金率查询应答
void CTraderSpi::OnRspQryInvestorMargin(CUstpFtdcInvestorMarginField *pInvestorMargin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
        lmice_error_print("[%d]查询投资者保证金率失败[%d] 错误原因：%s\n",
               nRequestID,
               pRspInfo->ErrorID,
               gbktoutf8( pRspInfo->ErrorMsg).c_str());
		return;
	}
	
	if (pInvestorMargin==NULL)
	{
		printf("没有查询到投资者保证金率\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("经纪公司编号=[%s]\n",pInvestorMargin->BrokerID);
	printf("合约代码=[%s]\n",pInvestorMargin->InstrumentID);
	printf("客户代码=[%s]\n",pInvestorMargin->ClientID);
	printf("多头占用保证金按比例=[%f]\n",pInvestorMargin->LongMarginRate);
	printf("多头保证金按手数=[%f]\n",pInvestorMargin->LongMarginAmt);
	printf("空头占用保证金按比例=[%f]\n",pInvestorMargin->ShortMarginRate);
	printf("空头保证金按手数=[%f]\n",pInvestorMargin->ShortMarginAmt);
	printf("-----------------------------\n");
	return ;

}


void CTraderSpi::OnRspQryComplianceParam(CUstpFtdcRspComplianceParamField *pRspComplianceParam, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
        printf("查询合规参数失败 错误原因：%s\n",gbktoutf8( pRspInfo->ErrorMsg).c_str());
		return;
	}
	
	if (pRspComplianceParam==NULL)
	{
		printf("没有查询到合规参数\n");
		return ;
	}
	printf("-----------------------------\n");
	printf("经纪公司编号=[%s]\n",pRspComplianceParam->BrokerID);
	printf("客户代码=[%s]\n",pRspComplianceParam->ClientID);
	printf("每日最大报单笔=[%d]\n",pRspComplianceParam->DailyMaxOrder);
	printf("每日最大撤单笔=[%d]\n",pRspComplianceParam->DailyMaxOrderAction);
	printf("每日最大错单笔=[%d]\n",pRspComplianceParam->DailyMaxErrorOrder);
	printf("每日最大报单手=[%d]\n",pRspComplianceParam->DailyMaxOrderVolume);
	printf("每日最大撤单手=[%d]\n",pRspComplianceParam->DailyMaxOrderActionVolume);
	printf("-----------------------------\n");
	return ;


}


int icount=0;
void CTraderSpi::OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus)
{
	if (pInstrumentStatus==NULL)
	{
		printf("没有合约状态信息\n");
		return ;
	}
	icount++;
	
	printf("-----------------------------\n");
	printf("交易所代码=[%s]\n",pInstrumentStatus->ExchangeID);
	printf("品种代码=[%s]\n",pInstrumentStatus->ProductID);
	printf("品种名称=[%s]\n",pInstrumentStatus->ProductName);
	printf("合约代码=[%s]\n",pInstrumentStatus->InstrumentID);
	printf("合约名称=[%s]\n",pInstrumentStatus->InstrumentName);
	printf("交割年份=[%d]\n",pInstrumentStatus->DeliveryYear);
	printf("交割月=[%d]\n",pInstrumentStatus->DeliveryMonth);
	printf("限价单最大下单量=[%d]\n",pInstrumentStatus->MaxLimitOrderVolume);
	printf("限价单最小下单量=[%d]\n",pInstrumentStatus->MinLimitOrderVolume);
	printf("市价单最大下单量=[%d]\n",pInstrumentStatus->MaxMarketOrderVolume);
	printf("市价单最小下单量=[%d]\n",pInstrumentStatus->MinMarketOrderVolume);
	
	printf("数量乘数=[%d]\n",pInstrumentStatus->VolumeMultiple);
	printf("报价单位=[%lf]\n",pInstrumentStatus->PriceTick);
	printf("币种=[%c]\n",pInstrumentStatus->Currency);
	printf("多头限仓=[%d]\n",pInstrumentStatus->LongPosLimit);
	printf("空头限仓=[%d]\n",pInstrumentStatus->ShortPosLimit);
	printf("跌停板价=[%lf]\n",pInstrumentStatus->LowerLimitPrice);
	printf("涨停板价=[%lf]\n",pInstrumentStatus->UpperLimitPrice);
	printf("昨结算=[%lf]\n",pInstrumentStatus->PreSettlementPrice);
	printf("合约交易状态=[%c]\n",pInstrumentStatus->InstrumentStatus);
	
	printf("创建日=[%s]\n",pInstrumentStatus->CreateDate);
	printf("上市日=[%s]\n",pInstrumentStatus->OpenDate);
	printf("到期日=[%s]\n",pInstrumentStatus->ExpireDate);
	printf("开始交割日=[%s]\n",pInstrumentStatus->StartDelivDate);
	printf("最后交割日=[%s]\n",pInstrumentStatus->EndDelivDate);
	printf("挂牌基准价=[%lf]\n",pInstrumentStatus->BasisPrice);
	printf("当前是否交易=[%d]\n",pInstrumentStatus->IsTrading);
	printf("基础商品代码=[%s]\n",pInstrumentStatus->UnderlyingInstrID);
	printf("持仓类型=[%c]\n",pInstrumentStatus->PositionType);
	printf("执行价=[%lf]\n",pInstrumentStatus->StrikePrice);
	printf("期权类型=[%c]\n",pInstrumentStatus->OptionsType);

	printf("-----------------------------\n");
	printf("[%d]",icount);
	return ;

}


void CTraderSpi::OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes)
{
	if (pInvestorAccountDepositRes==NULL)
	{
		printf("没有资金推送信息\n");
		return ;
	}

	printf("-----------------------------\n");
	printf("经纪公司编号=[%s]\n",pInvestorAccountDepositRes->BrokerID);
	printf("用户代码＝[%s]\n",pInvestorAccountDepositRes->UserID);
	printf("投资者编号=[%s]\n",pInvestorAccountDepositRes->InvestorID);
	printf("资金账号=[%s]\n",pInvestorAccountDepositRes->AccountID);
	printf("资金流水号＝[%s]\n",pInvestorAccountDepositRes->AccountSeqNo);
	printf("金额＝[%s]\n",pInvestorAccountDepositRes->Amount);
	printf("出入金方向＝[%s]\n",pInvestorAccountDepositRes->AmountDirection);
	printf("可用资金＝[%s]\n",pInvestorAccountDepositRes->Available);
	printf("结算准备金＝[%s]\n",pInvestorAccountDepositRes->Balance);
	printf("-----------------------------\n");
	return ;

}
