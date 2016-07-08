#include "fm2spi.h"
#include "lmice_trace.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


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

#define MODELNAME(s, m, t) do{\
    memset(symbol, 0, sizeof(symbol));  \
    strncat(symbol, m, sizeof(symbol)-1);   \
    strncat(symbol, t, sizeof(symbol) -1);  \
} while(0)

    //[modelname]-req-flatten
int CFemas2TraderSpi::init_trader() {
    int ret;
    char symbol[32];

    //Subscribe
    //收到请求后，下交易请求
    MODELNAME(symbol, m_model_name, "-req-orderInster");
    subscribe(symbol);
    register_cb(order_insert, symbol);

    //收到请求后,全部平仓
    MODELNAME(symbol, m_model_name, "-req-flatten");
    subscribe(symbol);
    register_cb(flatten_all, symbol);

}

///< Utilities
forceinline void login(CFemas2TraderSpi* spi)
{
    CUstpFtdcReqUserLoginField req;
    memset(&req, 0, sizeof req);
    //memcpy(req.BrokerID, m_fmconf.g_BrokerID, sizeof req.BrokerID);
    strcpy(req.BrokerID, spi->broker_id());
    //memcpy(req.UserID, m_fmconf.g_UserID, sizeof req.UserID);
    strcpy(req.UserID, spi->user_id());
    //memcpy(req.Password, m_fmconf.g_Password, sizeof req.Password);
    strcpy(req.Password, spi->password());
    /*memcpy(req.UserProductInfo, m_fmconf.g_ProductInfo, sizeof req.UserProductInfo);*/
    //strcpy(req.UserProductInfo, m_fmconf.g_ProductInfo);
    spi->trader()->ReqUserLogin(&req, spi->req_id());
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

void CFemas2TraderSpi::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    int64_t now;
    get_system_time(&now);

    publish("");
}

void CFemas2TraderSpi::OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}
