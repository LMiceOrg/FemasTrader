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
    //memcpy(req.BrokerID, m_fmconf.g_BrokerID, sizeof req.BrokerID);
    strncpy(req.BrokerID, spi->broker_id(), sizeof(req.BrokerID)-1);
    //memcpy(req.UserID, m_fmconf.g_UserID, sizeof req.UserID);
    strncpy(req.UserID, spi->user_id(), sizeof(req.UserID)-1);
    //memcpy(req.Password, m_fmconf.g_Password, sizeof req.Password);
    strncpy(req.Password, spi->password(), sizeof(req.Password)-1);
    /*memcpy(req.UserProductInfo, m_fmconf.g_ProductInfo, sizeof req.UserProductInfo);*/
    //strcpy(req.UserProductInfo, m_fmconf.g_ProductInfo);
    spi->trader()->ReqUserLogin(&req, spi->req_id());
}

forceinline void logout(CFemas2TraderSpi* spi) {
    CUstpFtdcReqUserLogoutField req;
    memset(&req, 0, sizeof req);
    strncpy(req.BrokerID, spi->broker_id(), sizeof(req.BrokerID)-1);
    strncpy(req.UserID, spi->user_id(), sizeof(req.UserID)-1);

    spi->trader()->ReqUserLogout(&req, spi->req_id());
}

forceinline void orderinsert(CFemas2TraderSpi* spi) {
    CUstpFtdcInputOrderField req;
    memset(&req, 0, sizeof req);
    strncpy(req.BrokerID, spi->broker_id(), sizeof(req.BrokerID)-1);
    strncpy(req.ExchangeID, spi->exchange_id(), sizeof(req.ExchangeID)-1);
    ///系统报单编号 。置空
    //TUstpFtdcOrderSysIDType	OrderSysID;
    strncpy(req.InvestorID, spi->investor_id(), sizeof(req.InvestorID)-1);
    strncpy(req.UserID, spi->user_id(), sizeof(req.UserID)-1);
}

CFemas2TraderSpi::CFemas2TraderSpi(CUstpFtdcTraderApi *pt, const char *name)
    :CLMSpi(name, -1)
{
    m_trader= pt;
    m_curid = 1;
    m_state = FMTRADER_UNKNOWN;
    m_user_id   =NULL;
    m_password=NULL;
    m_broker_id=NULL;
    m_front_address=NULL;
    m_investor_id=NULL;
    m_model_name = NULL;
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
    MODELNAME(symbol, m_model_name, "-tradeinstrument");
    subscribe(symbol);
    cb = static_cast<csymbol_callback>
            (&CFemas2TraderSpi::trade_instrument);
    register_cb(cb, symbol);

    //收到请求后，下交易请求
    MODELNAME(symbol, m_model_name, "-req-orderinstert");
    subscribe(symbol);
    cb = static_cast<csymbol_callback>
            (&CFemas2TraderSpi::order_insert);
    register_cb(cb, symbol);

    //收到请求后,全部平仓
    MODELNAME(symbol, m_model_name, "-req-flatten");
    subscribe(symbol);
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



}

void CFemas2TraderSpi::order_insert(const char *symbol, const void *addr, int size)
{
    (void)symbol;
    (void)addr;
    (void)size;
    lmice_critical_print("order insert\n");
}

void CFemas2TraderSpi::flatten_all(const char *symbol, const void *addr, int size)
{
    (void)symbol;
    (void)addr;
    (void)size;
    lmice_critical_print("flatten all\n");
}

void CFemas2TraderSpi::trade_instrument(const char *symbol, const void *addr, int size)
{
    (void)symbol;
    (void)addr;
    (void)size;
    lmice_critical_print("trade instrument\n");
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

void CFemas2TraderSpi::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

    lmice_info_print("RspOrder insert\n");
}

void CFemas2TraderSpi::OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    lmice_info_print("ResOder action\n");
}

void CFemas2TraderSpi::OnRtnTrade(CUstpFtdcTradeField *pTrade) {

}

void CFemas2TraderSpi::OnRtnOrder(CUstpFtdcOrderField *pOrder) {

}

void CFemas2TraderSpi::OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus) {

}

void CFemas2TraderSpi::OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes) {

}

void CFemas2TraderSpi::OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo) {

}

void CFemas2TraderSpi::OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo) {

}
