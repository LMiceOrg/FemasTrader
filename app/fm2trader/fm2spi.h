#ifndef FM2SPI_H
#define FM2SPI_H
#include "lmspi.h"
#include "USTPFtdcTraderApi.h"
#include "USTPFtdcUserApiStruct.h"
#include "strategy_status.h"

enum EFEMAS2TRADER {
    FMTRADER_UNKNOWN=0,
    FMTRADER_CONNECTED,
    FMTRADER_LOGIN,
    FMTRADER_DISCONNECTED
};

#define FM_TIME_LOG_MAX 65535 
enum FM_TIME_TYPE{
	fm_time_get_req = 0,
	fm_time_send_order,
	fm_time_get_rtn
};

typedef struct fm2_trader_postion{
	int m_buy_pos;
	int m_sell_pos;
}FM2_POS,*FM2_POS_P;

typedef struct fm2_trader_account{
	double m_curr_available;
}FM2_ACC,*FM2_ACC_P;

class CFemas2TraderSpi: public CLMSpi,
        public CUstpFtdcTraderSpi {
public:
     CFemas2TraderSpi(CUstpFtdcTraderApi *pt, const char* name);
    ~CFemas2TraderSpi();

    ///< Get properties
    //new request id
    int req_id();
    //trader
    CUstpFtdcTraderApi* trader() const;
	
    const char* user_id() const;
    const char* password() const;
    const char* broker_id() const;
    const char* front_address() const;
    const char* investor_id() const;
    const char* model_name() const;
    const char* exchange_id() const;
    void user_id(const char* id);
    void password(const char* id);
    void broker_id(const char* id);
    void front_address(const char* id);
    void investor_id(const char* id);
    void model_name(const char* id);
    void exchange_id(const char* id);

	const CUR_STATUS_P get_status();

    ///<
    int init_trader();
    void order_insert(const char* symbol, const void* addr, int size);
    void flatten_all(const char* symbol, const void* addr, int size);
    void trade_instrument(const char* symbol, const void* addr, int size);

    ///< Callbacks
    void OnFrontConnected();
    void OnFrontDisconnected(int nReason);
    void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    void OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //traderSPI
    void OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    void OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    void OnRtnTrade(CUstpFtdcTradeField *pTrade);
    void OnRtnOrder(CUstpFtdcOrderField *pOrder);
    void OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus);
    void OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes);

    void OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo);
    void OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo);

	void OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


    //QuerySPI
    //void OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryTrade(CUstpFtdcTradeField *pTrade, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryExchange(CUstpFtdcRspExchangeField *pRspExchange, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pUserInvestor, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryTradingCode(CUstpFtdcRspTradingCodeField *pTradingCode, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //void OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///合规参数查询应答
    //void OnRspQryComplianceParam(CUstpFtdcRspComplianceParamField *pRspComplianceParam, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///投资者手续费率查询应答
    //void OnRspQryInvestorFee(CUstpFtdcInvestorFeeField *pInvestorFee, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///投资者保证金率查询应答
    //void OnRspQryInvestorMargin(CUstpFtdcInvestorMarginField *pInvestorMargin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	int64_t m_time_log[FM_TIME_LOG_MAX][3];
	int m_time_log_pos;

private:
    CUstpFtdcTraderApi *m_trader;
    int m_curid;
    EFEMAS2TRADER m_state;

    const char* m_user_id;
    const char* m_password;
    const char* m_broker_id;
    const char* m_front_address;
    const char* m_investor_id;
    const char* m_model_name;
    const char* m_exchange_id;

	//current status
	//double m_org_available;
	CUR_STATUS m_trade_status;

};

#endif // FM2SPI_H

