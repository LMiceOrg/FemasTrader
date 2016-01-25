// TraderSpi.h: interface for the CTraderSpi class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRADERSPI_H__4B26E1C3_9EEE_4412_8C08_71BFB55CE6FF__INCLUDED_)
#define AFX_TRADERSPI_H__4B26E1C3_9EEE_4412_8C08_71BFB55CE6FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PublicFuncs.h"
class CTraderSpi : public CUstpFtdcTraderSpi  
{
public:
	CTraderSpi(CUstpFtdcTraderApi *pTrader);
	virtual ~CTraderSpi();

	virtual void OnFrontConnected();
	virtual void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//traderSPI
	virtual void OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserPasswordUpdate(CUstpFtdcUserPasswordUpdateField *pUserPasswordUpdate, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	virtual void OnRtnTrade(CUstpFtdcTradeField *pTrade);
	virtual void OnRtnOrder(CUstpFtdcOrderField *pOrder);
	virtual void OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus);
	virtual void OnRtnInvestorAccountDeposit(CUstpFtdcInvestorAccountDepositResField *pInvestorAccountDepositRes);

	virtual void OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo);
	virtual void OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo);

	//QuerySPI
	virtual void OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryTrade(CUstpFtdcTradeField *pTrade, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryExchange(CUstpFtdcRspExchangeField *pRspExchange, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pUserInvestor, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryTradingCode(CUstpFtdcRspTradingCodeField *pTradingCode, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///合规参数查询应答
	virtual void OnRspQryComplianceParam(CUstpFtdcRspComplianceParamField *pRspComplianceParam, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///投资者手续费率查询应答
	virtual void OnRspQryInvestorFee(CUstpFtdcInvestorFeeField *pInvestorFee, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///投资者保证金率查询应答
	virtual void OnRspQryInvestorMargin(CUstpFtdcInvestorMarginField *pInvestorMargin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	void Show(CUstpFtdcOrderField *pOrder);
	void Show(CUstpFtdcTradeField *pTrade);
	void Show(CUstpFtdcRspInstrumentField *pRspInstrument);
private:	
	CUstpFtdcTraderApi *m_pUserApi;
};

#endif // !defined(AFX_TRADERSPI_H__4B26E1C3_9EEE_4412_8C08_71BFB55CE6FF__INCLUDED_)
