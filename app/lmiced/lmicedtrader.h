#ifndef LMICEDTRADER_H
#define LMICEDTRADER_H
#include "ZeusingFtdcTraderApi.h"
//class LmicedTrader : public CZeusingFtdcTraderApi
//{
//public:
//    LmicedTrader();
//};
const int BUFLEN=512;
class LmicedTrader:public CZeusingFtdcTraderSpi
{
public:
    TZeusingFtdcBrokerIDType g_BrokerID;
    TZeusingFtdcUserIDType	g_UserID;
    TZeusingFtdcPasswordType	g_Password;
    char g_frontaddr[BUFLEN];
    int g_nOrdLocalID;
    char* g_pProductInfo;
    CZeusingFtdcTraderApi* m_pUserApi;
    void StartAutoOrder();
    void Show(CZeusingFtdcTradeField *pTrade);
    void Show(CZeusingFtdcOrderField *pOrder);
    void Show(CZeusingFtdcInstrumentField *pRspInstrument);
public:
    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    void OnFrontConnected();

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    ///@param nReason 错误原因
    ///        0x1001 网络读失败
    ///        0x1002 网络写失败
    ///        0x2001 接收心跳超时
    ///        0x2002 发送心跳失败
    ///        0x2003 收到错误报文
    void OnFrontDisconnected(int nReason);

    ///心跳超时警告。当长时间未收到报文时，该方法被调用。
    ///@param nTimeLapse 距离上次接收报文的时间
    void OnHeartBeatWarning(int nTimeLapse);

    ///客户端认证响应
    void OnRspAuthenticate(CZeusingFtdcRspAuthenticateField *pRspAuthenticateField, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


    ///登录请求响应
    void OnRspUserLogin(CZeusingFtdcRspUserLoginField *pRspUserLogin, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///登出请求响应
    void OnRspUserLogout(CZeusingFtdcUserLogoutField *pUserLogout, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///用户口令更新请求响应
    void OnRspUserPasswordUpdate(CZeusingFtdcUserPasswordUpdateField *pUserPasswordUpdate, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///资金账户口令更新请求响应
    void OnRspTradingAccountPasswordUpdate(CZeusingFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///报单录入请求响应
    void OnRspOrderInsert(CZeusingFtdcInputOrderField *pInputOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///预埋单录入请求响应
    void OnRspParkedOrderInsert(CZeusingFtdcParkedOrderField *pParkedOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///预埋撤单录入请求响应
    void OnRspParkedOrderAction(CZeusingFtdcParkedOrderActionField *pParkedOrderAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///报单操作请求响应
    void OnRspOrderAction(CZeusingFtdcInputOrderActionField *pInputOrderAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///查询最大报单数量响应
    void OnRspQueryMaxOrderVolume(CZeusingFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///投资者结算结果确认响应
    void OnRspSettlementInfoConfirm(CZeusingFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///删除预埋单响应
    void OnRspRemoveParkedOrder(CZeusingFtdcRemoveParkedOrderField *pRemoveParkedOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///删除预埋撤单响应
    void OnRspRemoveParkedOrderAction(CZeusingFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///执行宣告录入请求响应
    void OnRspExecOrderInsert(CZeusingFtdcInputExecOrderField *pInputExecOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///执行宣告操作请求响应
    void OnRspExecOrderAction(CZeusingFtdcInputExecOrderActionField *pInputExecOrderAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///报价录入请求响应
    void OnRspQuoteInsert(CZeusingFtdcInputQuoteField *pInputQuote, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///报价操作请求响应
    void OnRspQuoteAction(CZeusingFtdcInputQuoteActionField *pInputQuoteAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///询价操作应答
    void OnRspForQuote(CZeusingFtdcInputForQuoteField *pInputForQuote, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求资金内转响应
    void OnRspInternalTransfer(CZeusingFtdcInternalTransferField *pInternalTransfer, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询报单响应
    void OnRspQryOrder(CZeusingFtdcOrderField *pOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询成交响应
    void OnRspQryTrade(CZeusingFtdcTradeField *pTrade, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询投资者持仓响应
    void OnRspQryInvestorPosition(CZeusingFtdcInvestorPositionField *pInvestorPosition, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询资金账户响应
    void OnRspQryTradingAccount(CZeusingFtdcTradingAccountField *pTradingAccount, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询投资者响应
    void OnRspQryInvestor(CZeusingFtdcInvestorField *pInvestor, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询交易编码响应
    void OnRspQryTradingCode(CZeusingFtdcTradingCodeField *pTradingCode, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询合约保证金率响应
    void OnRspQryInstrumentMarginRate(CZeusingFtdcInstrumentMarginRateField *pInstrumentMarginRate, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询合约手续费率响应
    void OnRspQryInstrumentCommissionRate(CZeusingFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询交易所响应
    void OnRspQryExchange(CZeusingFtdcExchangeField *pExchange, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询合约响应
    void OnRspQryInstrument(CZeusingFtdcInstrumentField *pInstrument, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询行情响应
    void OnRspQryDepthMarketData(CZeusingFtdcDepthMarketDataField *pDepthMarketData, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询投资者结算结果响应
    void OnRspQrySettlementInfo(CZeusingFtdcSettlementInfoField *pSettlementInfo, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询转帐银行响应
    void OnRspQryTransferBank(CZeusingFtdcTransferBankField *pTransferBank, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询投资者持仓明细响应
    void OnRspQryInvestorPositionDetail(CZeusingFtdcInvestorPositionDetailField *pInvestorPositionDetail, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询投资者持仓明细响应
    void OnRspQryInvestorPositionCombineDetail(CZeusingFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///查询保证金监管系统经纪公司资金账户密钥响应
    void OnRspQryCFMMCTradingAccountKey(CZeusingFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询仓单折抵信息响应
    void OnRspQryEWarrantOffset(CZeusingFtdcEWarrantOffsetField *pEWarrantOffset, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询执行宣告响应
    void OnRspQryExecOrder(CZeusingFtdcExecOrderField *pExecOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询执行宣告操作响应
    void OnRspQryExecOrderAction(CZeusingFtdcExecOrderActionField *pExecOrderAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询错误执行宣告响应
    void OnRspQryErrExecOrder(CZeusingFtdcErrExecOrderField *pErrExecOrder, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询错误执行宣告操作响应
    void OnRspQryErrExecOrderAction(CZeusingFtdcErrExecOrderActionField *pErrExecOrderAction, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询转帐流水响应
    void OnRspQryTransferSerial(CZeusingFtdcTransferSerialField *pTransferSerial, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询银期签约关系响应
    void OnRspQryAccountregister(CZeusingFtdcAccountregisterField *pAccountregister, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///错误应答
    void OnRspError(CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///报单通知
    void OnRtnOrder(CZeusingFtdcOrderField *pOrder) ;

    ///成交通知
    void OnRtnTrade(CZeusingFtdcTradeField *pTrade) ;

    ///报单录入错误回报
    void OnErrRtnOrderInsert(CZeusingFtdcInputOrderField *pInputOrder, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///报单操作错误回报
    void OnErrRtnOrderAction(CZeusingFtdcOrderActionField *pOrderAction, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///合约交易状态通知
    void OnRtnInstrumentStatus(CZeusingFtdcInstrumentStatusField *pInstrumentStatus) ;

    ///交易通知
    void OnRtnTradingNotice(CZeusingFtdcTradingNoticeInfoField *pTradingNoticeInfo) ;

    ///提示条件单校验错误
    void OnRtnErrorConditionalOrder(CZeusingFtdcErrorConditionalOrderField *pErrorConditionalOrder) ;

    ///报价录入错误回报
    void OnErrRtnQuoteInsert(CZeusingFtdcInputQuoteField *pInputQuote, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///报价操作错误回报
    void OnErrRtnQuoteAction(CZeusingFtdcQuoteActionField *pQuoteAction, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///询价通知
    void OnRtnForQuote(CZeusingFtdcForQuoteField *pForQuote) ;

    ///询价回报
    void OnRtnExchRspForQuote(CZeusingFtdcExchRspForQuoteField *pExchRspForQuote) ;

    ///提示交易所询价失败
    void OnRtnErrExchRtnForQuote(CZeusingFtdcErrRtnExchRtnForQuoteField *pErrRtnExchRtnForQuote) ;

    ///请求查询签约银行响应
    void OnRspQryContractBank(CZeusingFtdcContractBankField *pContractBank, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询交易通知响应
    void OnRspQryTradingNotice(CZeusingFtdcTradingNoticeField *pTradingNotice, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询经纪公司交易参数响应
    void OnRspQryBrokerTradingParams(CZeusingFtdcBrokerTradingParamsField *pBrokerTradingParams, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///请求查询经纪公司交易算法响应
    void OnRspQryBrokerTradingAlgos(CZeusingFtdcBrokerTradingAlgosField *pBrokerTradingAlgos, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///银行发起银行资金转期货通知
    void OnRtnFromBankToFutureByBank(CZeusingFtdcRspTransferField *pRspTransfer) ;

    ///银行发起期货资金转银行通知
    void OnRtnFromFutureToBankByBank(CZeusingFtdcRspTransferField *pRspTransfer) ;

    ///银行发起冲正银行转期货通知
    void OnRtnRepealFromBankToFutureByBank(CZeusingFtdcRspRepealField *pRspRepeal) ;

    ///银行发起冲正期货转银行通知
    void OnRtnRepealFromFutureToBankByBank(CZeusingFtdcRspRepealField *pRspRepeal) ;

    ///期货发起银行资金转期货通知
    void OnRtnFromBankToFutureByFuture(CZeusingFtdcRspTransferField *pRspTransfer) ;

    ///期货发起期货资金转银行通知
    void OnRtnFromFutureToBankByFuture(CZeusingFtdcRspTransferField *pRspTransfer) ;

    ///系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
    void OnRtnRepealFromBankToFutureByFutureManual(CZeusingFtdcRspRepealField *pRspRepeal) ;

    ///系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
    void OnRtnRepealFromFutureToBankByFutureManual(CZeusingFtdcRspRepealField *pRspRepeal) ;

    ///期货发起查询银行余额通知
    void OnRtnQueryBankBalanceByFuture(CZeusingFtdcNotifyQueryAccountField *pNotifyQueryAccount) ;

    ///期货发起银行资金转期货错误回报
    void OnErrRtnBankToFutureByFuture(CZeusingFtdcReqTransferField *pReqTransfer, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///期货发起期货资金转银行错误回报
    void OnErrRtnFutureToBankByFuture(CZeusingFtdcReqTransferField *pReqTransfer, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///系统运行时期货端手工发起冲正银行转期货错误回报
    void OnErrRtnRepealBankToFutureByFutureManual(CZeusingFtdcReqRepealField *pReqRepeal, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///系统运行时期货端手工发起冲正期货转银行错误回报
    void OnErrRtnRepealFutureToBankByFutureManual(CZeusingFtdcReqRepealField *pReqRepeal, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///期货发起查询银行余额错误回报
    void OnErrRtnQueryBankBalanceByFuture(CZeusingFtdcReqQueryAccountField *pReqQueryAccount, CZeusingFtdcRspInfoField *pRspInfo) ;

    ///期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
    void OnRtnRepealFromBankToFutureByFuture(CZeusingFtdcRspRepealField *pRspRepeal) ;

    ///期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
    void OnRtnRepealFromFutureToBankByFuture(CZeusingFtdcRspRepealField *pRspRepeal) ;

    ///期货发起银行资金转期货应答
    void OnRspFromBankToFutureByFuture(CZeusingFtdcReqTransferField *pReqTransfer, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///期货发起期货资金转银行应答
    void OnRspFromFutureToBankByFuture(CZeusingFtdcReqTransferField *pReqTransfer, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///期货发起查询银行余额应答
    void OnRspQueryBankAccountMoneyByFuture(CZeusingFtdcReqQueryAccountField *pReqQueryAccount, CZeusingFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

    ///银行发起银期开户通知
    void OnRtnOpenAccountByBank(CZeusingFtdcOpenAccountField *pOpenAccount) ;

    ///银行发起银期销户通知
    void OnRtnCancelAccountByBank(CZeusingFtdcCancelAccountField *pCancelAccount) ;

    ///银行发起变更银行账号通知
    void OnRtnChangeAccountByBank(CZeusingFtdcChangeAccountField *pChangeAccount) ;
};

#endif // LMICEDTRADER_H
