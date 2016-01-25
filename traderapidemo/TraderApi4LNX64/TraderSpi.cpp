// TraderSpi.cpp: implementation of the CTraderSpi class.
//
//////////////////////////////////////////////////////////////////////

#include "TraderSpi.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


extern FILE * g_fpRecv;

CTraderSpi::CTraderSpi(CUstpFtdcTraderApi *pTrader):m_pUserApi(pTrader)
{

}

CTraderSpi::~CTraderSpi()
{

}


void CTraderSpi::OnFrontConnected()
{
	CUstpFtdcReqUserLoginField reqUserLogin;
	memset(&reqUserLogin,0,sizeof(CUstpFtdcReqUserLoginField));		
	strcpy(reqUserLogin.BrokerID,g_BrokerID);
	strcpy(reqUserLogin.UserID, g_UserID);
	strcpy(reqUserLogin.Password, g_Password);	
	strcpy(reqUserLogin.UserProductInfo,g_pProductInfo);		
	m_pUserApi->ReqUserLogin(&reqUserLogin, g_nOrdLocalID);	
	
	printf("请求登录，BrokerID=[%s]UserID=[%s]\n",g_BrokerID,g_UserID);
#ifdef WIN32
	Sleep(1000);
#else
	usleep(1000);
#endif
}


void CTraderSpi::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("登录失败...错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	g_nOrdLocalID=atoi(pRspUserLogin->MaxOrderLocalID)+1;
 	printf("-----------------------------\n");
 	printf("登录成功，最大本地报单号:%d\n",g_nOrdLocalID);
 	printf("-----------------------------\n");

 	StartAutoOrder();
}

void CTraderSpi::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("报单失败 错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	if(pInputOrder==NULL)
	{
		printf("没有报单数据\n");
		return;
	}
	printf("-----------------------------\n");
	printf("报单成功\n");
	printf("-----------------------------\n");
	return ;
	
}


void CTraderSpi::OnRtnTrade(CUstpFtdcTradeField *pTrade)
{
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
	printf("-----------------------------\n");
	printf("收到报单回报\n");
	Show(pOrder);
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("撤单失败 错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	if(pOrderAction==NULL)
	{
		printf("没有撤单数据\n");
		return;
	}
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
		printf("修改密码失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("报单错误回报失败 错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	if(pInputOrder==NULL)
	{
		printf("没有数据\n");
		return;
	}
	printf("-----------------------------\n");
	printf("报单错误回报\n");
	printf("-----------------------------\n");
	return ;
}
void CTraderSpi::OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("撤单错误回报失败 错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	if(pOrderAction==NULL)
	{
		printf("没有数据\n");
		return;
	}
	printf("-----------------------------\n");
	printf("撤单错误回报\n");
	printf("-----------------------------\n");
	return ;
}

void CTraderSpi::OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("查询报单失败 错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	if(pOrder==NULL)
	{
		printf("没有查询到报单数据\n");
		return;
	}
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
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("查询成交失败 错误原因：%s\n",pRspInfo->ErrorMsg);
		printf("-----------------------------\n");
		return;
	}
	if(pTrade==NULL)
	{
		printf("没有查询到成交数据");
		return;
	}
	Show(pTrade);
	return ;
}
void CTraderSpi::OnRspQryExchange(CUstpFtdcRspExchangeField *pRspExchange, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("查询交易所失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询投资者账户失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询可用投资者失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
	printf("-----------------------------\n");
	printf("交易所代码=[%s]\n",pRspInstrument->ExchangeID);
	printf("品种代码=[%s]\n",pRspInstrument->ProductID);
	printf("品种名称=[%s]\n",pRspInstrument->ProductName);
	printf("合约代码=[%s]\n",pRspInstrument->InstrumentID);
	printf("合约名称=[%s]\n",pRspInstrument->InstrumentName);
	printf("交割年份=[%d]\n",pRspInstrument->DeliveryYear);
	printf("交割月=[%d]\n",pRspInstrument->DeliveryMonth);
	printf("限价单最大下单量=[%d]\n",pRspInstrument->MaxLimitOrderVolume);
	printf("限价单最小下单量=[%d]\n",pRspInstrument->MinLimitOrderVolume);
	printf("市价单最大下单量=[%d]\n",pRspInstrument->MaxMarketOrderVolume);
	printf("市价单最小下单量=[%d]\n",pRspInstrument->MinMarketOrderVolume);
	
	printf("数量乘数=[%d]\n",pRspInstrument->VolumeMultiple);
	printf("报价单位=[%lf]\n",pRspInstrument->PriceTick);
	printf("币种=[%c]\n",pRspInstrument->Currency);
	printf("多头限仓=[%d]\n",pRspInstrument->LongPosLimit);
	printf("空头限仓=[%d]\n",pRspInstrument->ShortPosLimit);
	printf("跌停板价=[%lf]\n",pRspInstrument->LowerLimitPrice);
	printf("涨停板价=[%lf]\n",pRspInstrument->UpperLimitPrice);
	printf("昨结算=[%lf]\n",pRspInstrument->PreSettlementPrice);
	printf("合约交易状态=[%c]\n",pRspInstrument->InstrumentStatus);
	
	printf("创建日=[%s]\n",pRspInstrument->CreateDate);
	printf("上市日=[%s]\n",pRspInstrument->OpenDate);
	printf("到期日=[%s]\n",pRspInstrument->ExpireDate);
	printf("开始交割日=[%s]\n",pRspInstrument->StartDelivDate);
	printf("最后交割日=[%s]\n",pRspInstrument->EndDelivDate);
	printf("挂牌基准价=[%lf]\n",pRspInstrument->BasisPrice);
	printf("当前是否交易=[%d]\n",pRspInstrument->IsTrading);
	printf("基础商品代码=[%s]\n",pRspInstrument->UnderlyingInstrID);
	printf("持仓类型=[%c]\n",pRspInstrument->PositionType);
	printf("执行价=[%lf]\n",pRspInstrument->StrikePrice);
	printf("期权类型=[%c]\n",pRspInstrument->OptionsType);
	printf("-----------------------------\n");
	
}
void CTraderSpi::OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("查询交易编码失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询交易编码失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询投资者持仓 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询投资者手续费率失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询投资者保证金率失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
		printf("查询合规参数失败 错误原因：%s\n",pRspInfo->ErrorMsg);
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
