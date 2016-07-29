#include "fm2spi.h"
#include "lmice_trace.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

///< Utilities
forceinline void login(CFemas2TraderSpi* spi)
{
    CUstpFtdcReqUserLoginField req;
    memset(&req, 0, sizeof req);
    strncpy(req.BrokerID, spi->broker_id(), sizeof(req.BrokerID)-1);
    strncpy(req.UserID, spi->user_id(), sizeof(req.UserID)-1);
    strncpy(req.Password, spi->password(), sizeof(req.Password)-1);
    spi->trader()->ReqUserLogin(&req, spi->req_id());
}

forceinline void logout(CFemas2TraderSpi* spi) {
    CUstpFtdcReqUserLogoutField req;
    memset(&req, 0, sizeof req);
    strncpy(req.BrokerID, spi->broker_id(), sizeof(req.BrokerID)-1);
    strncpy(req.UserID, spi->user_id(), sizeof(req.UserID)-1);

    spi->trader()->ReqUserLogout(&req, spi->req_id());
}

forceinline void orderinsert(CFemas2TraderSpi* spi, CUstpFtdcInputOrderField* preq) {
	int local_id = spi->req_id();
    CUstpFtdcInputOrderField& req = *preq;
    strcpy(req.BrokerID, spi->broker_id());
    strcpy(req.ExchangeID, spi->exchange_id());
    strcpy(req.InvestorID, spi->investor_id());
    strcpy(req.UserID, spi->user_id());
	sprintf(req.UserOrderLocalID, "%012d", local_id);

/*	
	strcpy(req.InstrumentID, "rb1610");
	req.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
	req.Direction = USTP_FTDC_D_Buy;
	//req.OffsetFlag = USTP_FTDC_OF_Close;
	req.OffsetFlag = USTP_FTDC_OF_CloseToday;
	req.HedgeFlag = USTP_FTDC_CHF_Speculation;
	req.LimitPrice = 2351;
	req.Volume = 1;
	req.TimeCondition = USTP_FTDC_TC_IOC;
	req.VolumeCondition = USTP_FTDC_VC_AV;
	req.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
	int local_id = spi->req_id();
	
	printf("======= order insert ======");
	printf("BrokerID:%s\n", req.BrokerID);
	printf("ExchangeID:%s\n", req.ExchangeID);
	printf("InvestorID:%s\n", req.InvestorID);
	printf("ins_name:%s\n", req.InstrumentID);
*/
	spi->trader()->ReqOrderInsert(&req, local_id);
/*
	int64_t systime = 0;
	get_system_time(&systime);
	printf("\n=========real order insert:=========\n");
	printf("time:%ld\n", systime/10);
	printf("ins:%s\n", preq->InstrumentID);
	printf("price:%lf\n", preq->LimitPrice);
	printf("volume:%d\n", preq->Volume);
	printf("direction:%c\n", preq->Direction);
	printf("OffsetFlag:%c\n", preq->OffsetFlag);
*/
	
}

forceinline void orderaction(CFemas2TraderSpi* spi, CUstpFtdcOrderActionField* preq) 
{

}

forceinline void investoraccount(CFemas2TraderSpi* spi)
{
	CUstpFtdcQryInvestorAccountField qry;
	memset( &qry, 0, sizeof(CUstpFtdcQryInvestorAccountField) );
	strcpy(qry.BrokerID, spi->broker_id());
	strcpy(qry.UserID, spi->user_id());
	
	spi->trader()->ReqQryInvestorAccount( &qry, spi->req_id());
}

forceinline void investorposition(CFemas2TraderSpi* spi)
{
	CUstpFtdcQryInvestorPositionField qry;
	memset( &qry, 0, sizeof(CUstpFtdcQryInvestorPositionField) );
	strcpy( qry.BrokerID, spi->broker_id() );
	strcpy( qry.UserID, spi->user_id() );
	strcpy( qry.InvestorID, spi->investor_id() );
	int local_id = spi->req_id();
	printf("local_id:%d\n", local_id);
	
	spi->trader()->ReqQryInvestorPosition( &qry, local_id );
}

forceinline void hard_flatten(CFemas2TraderSpi* spi)
{
	const CUR_STATUS_P ptr_cur_st = spi->get_status();
	CUstpFtdcInputOrderField req;
	memset( &req, 0, sizeof(CUstpFtdcInputOrderField) );
	strcpy(req.InstrumentID, ptr_cur_st->m_ins_name);
	req.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
	req.OffsetFlag = USTP_FTDC_OF_CloseToday;
	req.HedgeFlag = USTP_FTDC_CHF_Speculation;
	req.TimeCondition = USTP_FTDC_TC_IOC;
	req.VolumeCondition = USTP_FTDC_VC_AV;
	req.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
	
	req.Volume = ptr_cur_st->m_pos.m_buy_pos;
	req.Direction = USTP_FTDC_D_Sell;
	req.LimitPrice = ptr_cur_st->m_md.m_down_price;
	if( req.Volume > 0 )
	{
		orderinsert( spi, &req );
	}

	req.Volume = ptr_cur_st->m_pos.m_sell_pos;
	req.Direction = USTP_FTDC_D_Buy;
	req.LimitPrice = ptr_cur_st->m_md.m_up_price;
	if( req.Volume > 0 )
	{
		orderinsert( spi, &req );
	}
	
}

forceinline void soft_flatten(CFemas2TraderSpi* spi)
{
	const CUR_STATUS_P ptr_cur_st = spi->get_status();
	CUstpFtdcInputOrderField req;
	
	if( ptr_cur_st->m_pos.m_buy_pos == ptr_cur_st->m_pos.m_sell_pos )
	{
		return;
	}

	memset( &req, 0, sizeof(CUstpFtdcInputOrderField) );
	strcpy(req.InstrumentID, ptr_cur_st->m_ins_name);
	req.OrderPriceType = USTP_FTDC_OPT_LimitPrice;
	req.OffsetFlag = USTP_FTDC_OF_CloseToday;
	req.HedgeFlag = USTP_FTDC_CHF_Speculation;
	req.TimeCondition = USTP_FTDC_TC_IOC;
	req.VolumeCondition = USTP_FTDC_VC_AV;
	req.ForceCloseReason = USTP_FTDC_FCR_NotForceClose;

	if( ptr_cur_st->m_pos.m_buy_pos > ptr_cur_st->m_pos.m_sell_pos )
	{
		req.Volume = ptr_cur_st->m_pos.m_buy_pos - ptr_cur_st->m_pos.m_sell_pos;
		req.Direction = USTP_FTDC_D_Sell;
		req.LimitPrice = ptr_cur_st->m_md.m_down_price;
	}
	else
	{
		req.Volume = ptr_cur_st->m_pos.m_sell_pos - ptr_cur_st->m_pos.m_buy_pos;
		req.Direction = USTP_FTDC_D_Buy;
		req.LimitPrice = ptr_cur_st->m_md.m_up_price;
	}
	orderinsert( spi, &req );
	
}

CFemas2TraderSpi::CFemas2TraderSpi(CUstpFtdcTraderApi *pt, const char *name)
    :CLMSpi(name, -1)
{
    m_trader= pt;
    m_curid = 0;
    m_state = FMTRADER_UNKNOWN;
    m_user_id = NULL;
    m_password = NULL;
    m_broker_id = NULL;
    m_front_address = NULL;
    m_investor_id = NULL;
    m_model_name = NULL;

	memset( &m_trade_status, 0, sizeof(CUR_STATUS));
	memset( m_time_log, 0, FM_TIME_LOG_MAX*3*sizeof(int64_t));
    m_time_log_pos = -1;
	
}

CFemas2TraderSpi::~CFemas2TraderSpi() {
    delete_spi();
}

int CFemas2TraderSpi::req_id()
{
    return ++m_curid;
}

CUstpFtdcTraderApi* CFemas2TraderSpi::trader() const {
    return m_trader;
}

const char* CFemas2TraderSpi::user_id() const {
    return m_user_id;
}

const char* CFemas2TraderSpi::password() const {
    return m_password;
}

const char* CFemas2TraderSpi::broker_id() const {
    return m_broker_id;
}
const char* CFemas2TraderSpi::front_address() const {
    return m_front_address;
}

const char* CFemas2TraderSpi::investor_id() const {
    return m_investor_id;
}
const char* CFemas2TraderSpi::model_name() const {
    return m_model_name;
}

const char *CFemas2TraderSpi::exchange_id() const
{
    return m_exchange_id;
}
void CFemas2TraderSpi::user_id(const char* id) {
    m_user_id = id;
}

void CFemas2TraderSpi::password(const char* id) {
    m_password = id;
}

void CFemas2TraderSpi::broker_id(const char* id) {
    m_broker_id = id;
}

void CFemas2TraderSpi::front_address(const char* id) {
    m_front_address = id;
}

void CFemas2TraderSpi::investor_id(const char* id) {
    m_investor_id = id;
}

void CFemas2TraderSpi::model_name(const char* id) {
    m_model_name = id;
}

void CFemas2TraderSpi::exchange_id(const char *id)
{
    m_exchange_id = id;
}

const CUR_STATUS_P CFemas2TraderSpi::get_status()
{
	const CUR_STATUS_P tmp = &m_trade_status;
	return tmp;
}

#define MODELNAME(s, m, t) do{\
    memset(symbol, 0, sizeof(symbol));  \
    strncat(symbol, m, sizeof(symbol)-1);   \
    strncat(symbol, t, sizeof(symbol) -1);  \
} while(0)

    //[modelname]-req-flatten
int CFemas2TraderSpi::init_trader() {
    int ret;
    char symbol[32];
    csymbol_callback cb;
	
    //Subscribe
    //收到交易品种,创建交易日志
    MODELNAME(symbol, m_model_name, "TradeInstrument");
    subscribe(symbol);
	lmice_info_print( "subscribe symbol: %s\n", symbol );
    cb = static_cast<csymbol_callback>
            (&CFemas2TraderSpi::trade_instrument);
    register_cb(cb, symbol);

    //收到请求后，下交易请求
    MODELNAME(symbol, m_model_name, "OrderInsert");
    subscribe(symbol);
	lmice_info_print( "subscribe symbol: %s\n", symbol );
    cb = static_cast<csymbol_callback>
            (&CFemas2TraderSpi::order_insert);
    register_cb(cb, symbol);

    //收到请求后,全部平仓
    MODELNAME(symbol, m_model_name, "Flatten");
    subscribe(symbol);
	lmice_info_print( "subscribe symbol: %s\n", symbol );
    cb = static_cast<csymbol_callback>
            (&CFemas2TraderSpi::flatten_all);
    register_cb(cb, symbol);

    //Publish
    //下单反馈(Femas)
    MODELNAME(symbol, m_model_name, "-rsp-orderinsert");
    publish(symbol);
    //撤单反馈(Femas)
    MODELNAME(symbol, m_model_name, "-rsp-orderaction");
    publish(symbol);

    //成交反馈(交易所)
    MODELNAME(symbol, m_model_name, "-rtn-trade");
    publish(symbol);

    //下单反馈(交易所)
    MODELNAME(symbol, m_model_name, "-rtn-order");
    publish(symbol);

	MODELNAME(symbol, m_model_name, "Position");
	lmice_info_print( "publish symbol: %s\n", symbol );
	publish(symbol);

	MODELNAME(symbol, m_model_name, "Account");
	lmice_info_print( "publish symbol: %s\n", symbol );
	publish(symbol);

}

void CFemas2TraderSpi::order_insert(const char *symbol, const void *addr, int size)
{
    (void)symbol;
    (void)addr;
    (void)size;

	int64_t systime = 0;
	get_system_time(&systime);
	
	m_time_log[++m_time_log_pos][fm_time_get_req] = systime/10;
	
    if(size != sizeof(CUstpFtdcInputOrderField)) {
        lmice_error_print("Order insert incorrect size\n");
        return;
    }
    CUstpFtdcInputOrderField req;
    memcpy(&req, addr, size);

	if( req.LimitPrice == 0 )
	{
		return;
	}

    orderinsert(this, &req);

	get_system_time(&systime);
	m_time_log[m_time_log_pos][fm_time_send_order] = systime/10;

    lmice_critical_print("order insert\n");
}

#define HARD_FLATTEN
void CFemas2TraderSpi::flatten_all(const char *symbol, const void *addr, int size)
{
    (void)symbol;
    (void)addr;
    (void)size;
    lmice_critical_print("flatten all\n");
	
#ifdef SOFT_FLATTEN
	soft_flatten(this);
#endif

#ifdef HARD_FLATTEN
	hard_flatten(this);
#endif

	printf("=======time log: %d=======\n", m_time_log_pos);
	int i = 0;
	for(; i<m_time_log_pos-1; i++ )
	{
		{
			printf("\t%ld\t\t%ld\t\t%ld\n", m_time_log[i][fm_time_get_req], m_time_log[i][fm_time_send_order], m_time_log[i][fm_time_get_rtn] );
		}
	}

	quit();	
}

void CFemas2TraderSpi::trade_instrument(const char *symbol, const void *addr, int size)
{
    (void)symbol;
    (void)addr;
    (void)size;
    lmice_critical_print("====trade instrument====\n");
	strncpy( m_trade_status.m_ins_name, (const char *)addr, size );
	printf("trade instrument: %s\n", m_trade_status.m_ins_name );
	lmice_info_print("get investor account state\n");
	investoraccount(this);
	//investorposition(this);
}

///< Callbacks

void CFemas2TraderSpi::OnFrontConnected()
{
    m_state = FMTRADER_LOGIN;
    lmice_info_print("Femas2Trader connected, do login\n");
    login(this);
}

void CFemas2TraderSpi::OnFrontDisconnected(int nReason)
{
    m_state = FMTRADER_DISCONNECTED;
    lmice_info_print("Femas2Trader disconnected");
}

void CFemas2TraderSpi::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    (void)pRspUserLogin;
    (void)nRequestID;
    (void)bIsLast;
    if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
    {

        lmice_error_print("Login failed(%s0 as error id[%d]\n",gbktoutf8( pRspInfo->ErrorMsg).c_str(),
                          pRspInfo->ErrorID );
        m_state = FMTRADER_CONNECTED;
        return;
    }
    lmice_info_print("Login successed\n");
	m_curid = atoi(pRspUserLogin->MaxOrderLocalID)+1;

/*
	CUstpFtdcInputOrderField req;
	memset(&req, 0, sizeof(CUstpFtdcInputOrderField));
	orderinsert(this, &req);
*/	
//	lmice_info_print("get investor account state\n");
//	investoraccount(this);
	
}

void CFemas2TraderSpi::OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    (void)pRspUserLogout;
    (void)nRequestID;
    (void)bIsLast;
    if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
    {

        lmice_error_print("Logout failed(%s0 as error id[%d]\n",gbktoutf8( pRspInfo->ErrorMsg).c_str(),
                          pRspInfo->ErrorID );
        m_state = FMTRADER_CONNECTED;
        return;
    }
    lmice_info_print("logout\n");
}

void CFemas2TraderSpi::OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
    {

        lmice_error_print("Logout failed(%s0 as error id[%d]\n",gbktoutf8( pRspInfo->ErrorMsg).c_str(),
                          pRspInfo->ErrorID );
        m_state = FMTRADER_CONNECTED;
        return;
    }
	lmice_info_print("OnRspQryInvestorAccount\n");

	printf("Request ID:%d\n", nRequestID);
	printf("BrokerID:%s\n", pRspInvestorAccount->BrokerID);
	printf("InvestorID:%s\n", pRspInvestorAccount->InvestorID);
	printf("AccountID:%s\n", pRspInvestorAccount->AccountID);
	printf("PreBalance:%lf\n", pRspInvestorAccount->PreBalance);
	printf("Deposit:%lf\n", pRspInvestorAccount->Deposit);
	printf("Withdraw:%lf\n", pRspInvestorAccount->Withdraw);
	printf("FrozenMargin:%lf\n", pRspInvestorAccount->FrozenMargin);
	printf("FrozenFee:%lf\n", pRspInvestorAccount->FrozenFee);
	printf("FrozenPremium:%lf\n", pRspInvestorAccount->FrozenPremium);
	printf("Fee:%lf\n", pRspInvestorAccount->Fee);
	printf("CloseProfit:%lf\n", pRspInvestorAccount->CloseProfit);
	printf("PositionProfit:%lf\n", pRspInvestorAccount->PositionProfit);
	printf("Available:%lf\n", pRspInvestorAccount->Available);
	printf("LongFrozenMargin:%lf\n", pRspInvestorAccount->LongFrozenMargin);
	printf("ShortFrozenMargin:%lf\n", pRspInvestorAccount->ShortFrozenMargin);
	printf("LongMargin:%lf\n", pRspInvestorAccount->LongMargin);
	printf("ShortMargin:%lf\n", pRspInvestorAccount->ShortMargin);
	printf("ReleaseMargin:%lf\n", pRspInvestorAccount->ReleaseMargin);
	printf("DynamicRights:%lf\n", pRspInvestorAccount->DynamicRights);
	printf("TodayInOut:%lf\n", pRspInvestorAccount->TodayInOut);
	printf("Margin:%lf\n", pRspInvestorAccount->Margin);
	printf("Premium:%lf\n", pRspInvestorAccount->Premium);
	printf("Risk:%lf\n", pRspInvestorAccount->Risk);
	printf("BeLast:%d\n", bIsLast);

	m_trade_status.m_acc.m_count_id = atoi(pRspInvestorAccount->AccountID);
	m_trade_status.m_acc.m_left_cash = 0;
	m_trade_status.m_acc.m_valid_val = pRspInvestorAccount->Available;
	m_trade_status.m_acc.m_total_val = pRspInvestorAccount->Available + pRspInvestorAccount->ShortMargin + pRspInvestorAccount->LongMargin;
	
	if( true == bIsLast )
	{
		lmice_info_print("get investor postion\n");
		sleep(1);
		investorposition(this);
	}
	
}

void CFemas2TraderSpi::OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
    {
        lmice_error_print("Logout failed(%s0 as error id[%d]\n",gbktoutf8( pRspInfo->ErrorMsg).c_str(),
                          pRspInfo->ErrorID );
        m_state = FMTRADER_CONNECTED;
        return;
    }
	lmice_info_print("OnRspQryInvestorPosition\n");


	if( pRspInvestorPosition == NULL )
	{
		printf("==== NULL pRspInvestorPosition ==\n");
	}
	else
	{
		printf("Request ID:%d\n", nRequestID);
		printf("BrokerID:%s\n", pRspInvestorPosition->BrokerID);
		printf("InvestorID:%s\n", pRspInvestorPosition->InvestorID);
		printf("Direction:%c\n", pRspInvestorPosition->Direction);
		printf("HedgeFlag:%c\n", pRspInvestorPosition->HedgeFlag);
		printf("UsedMargin:%lf\n", pRspInvestorPosition->UsedMargin);
		printf("Position:%d\n", pRspInvestorPosition->Position);
		printf("PositionCost:%lf\n", pRspInvestorPosition->PositionCost);
		printf("YdPosition:%d\n", pRspInvestorPosition->YdPosition);
		printf("YdPositionCost:%lf\n", pRspInvestorPosition->YdPositionCost);
		printf("FrozenMargin:%lf\n", pRspInvestorPosition->FrozenMargin);
		printf("FrozenPosition:%d\n", pRspInvestorPosition->FrozenPosition);
		printf("FrozenClosing:%d\n", pRspInvestorPosition->FrozenClosing);
		printf("FrozenPremium:%lf\n", pRspInvestorPosition->FrozenPremium);
		printf("LastTradeID:%s\n", pRspInvestorPosition->LastTradeID);
		printf("LastOrderLocalID:%s\n", pRspInvestorPosition->LastOrderLocalID);
		printf("Currency:%s\n", pRspInvestorPosition->Currency);
		
		if( USTP_FTDC_D_Buy == pRspInvestorPosition->Direction )
		{
			m_trade_status.m_pos.m_yd_buy_pos = pRspInvestorPosition->YdPosition;
			m_trade_status.m_pos.m_buy_pos = pRspInvestorPosition->Position;
		}
		else if( USTP_FTDC_D_Sell == pRspInvestorPosition->Direction )
		{
			m_trade_status.m_pos.m_yd_sell_pos = pRspInvestorPosition->YdPosition;
			m_trade_status.m_pos.m_sell_pos = pRspInvestorPosition->Position;
		}

	}

	//update current status to stragegyht
	if( bIsLast )
	{
		//to be add,add trade insturment message and update to st
		m_trade_status.m_md.fee_rate = 0.000101 + 0.0000006;
		m_trade_status.m_md.m_up_price = 2814;
		m_trade_status.m_md.m_down_price = 2495;
		m_trade_status.m_md.m_last_price = 0;
		m_trade_status.m_md.m_multiple = 10;

		char symbol[32];
		MODELNAME(symbol, m_model_name, "Account");
		lmice_info_print("update account status to strategyht,flag:%s\n", symbol);
		send( symbol, &m_trade_status, sizeof(CUR_STATUS));
	}
	
}

void CFemas2TraderSpi::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    lmice_info_print("RspOrder insert\n");
	
	if (pInputOrder==NULL )
	{
			printf("û�в�ѯ����Լ����\n");
			return ;
	}
	
	if( pRspInfo != NULL )
	{
			printf("id:%d\n", pRspInfo->ErrorID);
			printf("reason:%s\n", gbktoutf8(pRspInfo->ErrorMsg).c_str());
	}
	
}

void CFemas2TraderSpi::OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    lmice_info_print("ResOder action\n");
}

void CFemas2TraderSpi::OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	lmice_info_print("OnRspError\n");
	
	if( pRspInfo != NULL )
	{
		printf("id:%d\n", pRspInfo->ErrorID);
		printf("req id:%d\n", nRequestID);
		printf("reason:%s\n", gbktoutf8(pRspInfo->ErrorMsg).c_str());
	}

}

void CFemas2TraderSpi::OnRtnTrade(CUstpFtdcTradeField *pTrade) 
{
	lmice_info_print("OnRtnTrade\n");
	int update_flag = 0;

	int64_t systime = 0;
	get_system_time(&systime);

	m_time_log[m_time_log_pos][fm_time_get_rtn] = systime/10;

/*
	printf("BrokerID:%s\n", pTrade->BrokerID);
	printf("ExchangeID:%s\n", pTrade->ExchangeID);
	printf("TradingDay:%s\n", pTrade->TradingDay);
	printf("ParticipantID:%s\n", pTrade->ParticipantID);
	printf("SeatID:%s\n", pTrade->SeatID);
	printf("InvestorID:%s\n", pTrade->InvestorID);
	printf("ClientID:%s\n", pTrade->ClientID);
	printf("UserID:%s\n", pTrade->UserID);
	printf("TradeID:%s\n", pTrade->TradeID);
	printf("OrderSysID:%s\n", pTrade->OrderSysID);
	printf("UserOrderLocalID:%s\n", pTrade->UserOrderLocalID);
	printf("InstrumentID:%s\n", pTrade->InstrumentID);
	printf("Direction:%c\n", pTrade->Direction);
	printf("OffsetFlag:%c\n", pTrade->OffsetFlag);
	printf("HedgeFlag:%c\n", pTrade->HedgeFlag);
	printf("TradePrice:%lf\n", pTrade->TradePrice);
	printf("TradeVolume:%ld\n", pTrade->TradeVolume);
	printf("TradeTime:%s\n", pTrade->TradeTime);
	printf("ClearingPartID:%s\n", pTrade->ClearingPartID);
*/

	if( USTP_FTDC_OF_Open == pTrade->OffsetFlag )
	{
		if( USTP_FTDC_D_Buy == pTrade->Direction ) 
		{
			m_trade_status.m_pos.m_buy_pos += pTrade->TradeVolume;
			m_trade_status.m_acc.m_left_cash -= pTrade->TradeVolume * m_trade_status.m_md.m_multiple * pTrade->TradePrice * ( 1 + m_trade_status.m_md.fee_rate );
		}
		else if( USTP_FTDC_D_Sell == pTrade->Direction )
		{
			m_trade_status.m_pos.m_sell_pos += pTrade->TradeVolume;
			m_trade_status.m_acc.m_left_cash += pTrade->TradeVolume * m_trade_status.m_md.m_multiple * pTrade->TradePrice * ( 1 - m_trade_status.m_md.fee_rate );
		}
		update_flag = 1;
	}
	else if( USTP_FTDC_OF_CloseToday == pTrade->OffsetFlag )
	{
		if( USTP_FTDC_D_Buy == pTrade->Direction ) 
		{
			m_trade_status.m_pos.m_sell_pos -= pTrade->TradeVolume;
			m_trade_status.m_acc.m_left_cash -= pTrade->TradeVolume * m_trade_status.m_md.m_multiple * pTrade->TradePrice * ( 1 + m_trade_status.m_md.fee_rate );
		}
		else if( USTP_FTDC_D_Sell == pTrade->Direction )
		{
			m_trade_status.m_pos.m_buy_pos -= pTrade->TradeVolume;
			m_trade_status.m_acc.m_left_cash += pTrade->TradeVolume * m_trade_status.m_md.m_multiple * pTrade->TradePrice * ( 1 - m_trade_status.m_md.fee_rate );
		}
		update_flag = 1;
	}
	else if( USTP_FTDC_OF_CloseYesterday == pTrade->OffsetFlag )
	{
		if( USTP_FTDC_D_Buy == pTrade->Direction ) 
		{
			m_trade_status.m_pos.m_yd_buy_pos -= pTrade->TradeVolume;
		}
		else if( USTP_FTDC_D_Sell == pTrade->Direction )
		{
			m_trade_status.m_pos.m_yd_sell_pos -= pTrade->TradeVolume;
		}
	}
	else
	{
		lmice_error_print( "unknow offsetflag\n" );
	}

	if( update_flag )
	{
		char symbol[32];
		MODELNAME(symbol, m_model_name, "Account");
		send( symbol, &m_trade_status, sizeof(CUR_STATUS));
	}
	printf("\n== send account status==\n");
	printf("m_buy_pos:%d\n", m_trade_status.m_pos.m_buy_pos);
	printf("m_sell_pos:%d\n", m_trade_status.m_pos.m_sell_pos);
	printf("m_left_cash:%lf\n", m_trade_status.m_acc.m_left_cash);

}

void CFemas2TraderSpi::OnRtnOrder(CUstpFtdcOrderField *pOrder)
{
	lmice_info_print("RtnOrder insert\n");
}

void CFemas2TraderSpi::OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus) {

}

void CFemas2TraderSpi::OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes) {

}

void CFemas2TraderSpi::OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo) {

}

void CFemas2TraderSpi::OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo) {

}
