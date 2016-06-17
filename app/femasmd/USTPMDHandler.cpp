///@system	 飞马行情接受系统
///@company  上海金融期货信息技术有限公司
///@file	 USTPMDHandler.h
///@brief	 api实现
///@history 
///20130502: 徐忠华 创建
//////////////////////////////////////////////////////////////////////

#include "USTPMDHandler.h"

extern char *INI_FILE_NAME;

CUSTPMDHandler::CUSTPMDHandler(CUstpFtdcMduserApi *pUserApi,char *  number) : m_pUserApi(pUserApi) 
{
	strcpy(m_number,number);
	if (!tf.Open(INI_FILE_NAME))
	{
		printf("不能打开配置文件<config.ini>\n");
		exit(-1000);
	}
	tf.ReadString(m_number,"OutPutFile","output.csv",tmp,sizeof(tmp)-1);
	m_pFileOutput = fopen(tmp,"wt");
	tf.ReadString(m_number,"OutPutFile","output.csv",tmp,sizeof(tmp)-1);
	m_pFileOutput = fopen(tmp,"wt");
}

CUSTPMDHandler::~CUSTPMDHandler() 
{
	fclose(m_pFileOutput);
}

// 当客户端与行情发布服务器建立起通信连接，客户端需要进行登录
void CUSTPMDHandler::OnFrontConnected()
{
    printf("Connected\n");

    CUstpFtdcReqUserLoginField reqUserLogin;
    strcpy(reqUserLogin.TradingDay, m_pUserApi->GetTradingDay());
	tf.ReadString(m_number,"BrokerID","",tmp,sizeof(tmp)-1);
	strcpy(reqUserLogin.BrokerID,tmp);
	tf.ReadString(m_number,"UserID","",tmp,sizeof(tmp)-1);
    strcpy(reqUserLogin.UserID, tmp);
	tf.ReadString(m_number,"PassWD","",tmp,sizeof(tmp)-1);
    strcpy(reqUserLogin.Password,tmp);
    strcpy(reqUserLogin.UserProductInfo, "mduserdemo v1.0");
    m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
}

// 当客户端与行情发布服务器通信连接断开时，该方法被调用
void CUSTPMDHandler::OnFrontDisconnected() 
{
	// 当发生这个情况后，API会自动重新连接，客户端可不做处理
    printf("OnFrontDisconnected.\n");
}

void CUSTPMDHandler::OnRspQryTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	printf("OnRspQryTopic: %d ,%d\n",pDissemination->SequenceSeries,pDissemination->SequenceNo);
}

// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
void CUSTPMDHandler::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
    printf("\nOnRspUserLogin:\n");
	printf("UserID:[%s] \n",pRspUserLogin->UserID);
	printf("ParticipantID:[%s] \n",pRspUserLogin->BrokerID);
    printf("DataCenterID:[%d] \n",pRspUserLogin->DataCenterID);
	printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);

    if (pRspInfo->ErrorID != 0)
    {
        // 端登失败，客户端需进行错误处理
        printf("Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
    }
 
	//订阅合约信息
	// 	char* contracts[3]={"","",""};
	// 	contracts[0]="*";
	// 	contracts[1]="IF1212";
	// 	contracts[2]="IF1303";
	//	m_pUserApi->SubMarketData(contracts, 3);
	
	int subnum = tf.ReadInt(m_number,"SubInsNum",0);
	char** contracts = new char*[subnum];
	int i=0;
	char instmp[128];
	for(;i<subnum;i++)
	{
		sprintf(instmp,"SubIns%d",i+1);
		tf.ReadString(m_number,instmp,"",tmp,sizeof(tmp)-1);
		contracts[i]=new char[strlen(tmp)+1];
		strncpy(contracts[i],tmp,strlen(tmp)+1);
        printf("sub : %s\n",instmp);
	}
	m_pUserApi->SubMarketData(contracts, subnum);

	// 	char* uncontracts[3]={"","",""};
	// 	uncontracts[0]="IF1211";
	// 	uncontracts[1]="IF1212";
	// 	uncontracts[2]="IF1303";
	//	m_pUserApi->UnSubMarketData(uncontracts, 3);

	int unsubnum = tf.ReadInt(m_number,"UnSubInsNum",0);
	char** uncontracts = new char*[unsubnum];
	i=0;
	for(;i<unsubnum;i++)
	{
		sprintf(instmp,"UnSubIns%d",i+1);
		tf.ReadString(m_number,instmp,"",tmp,sizeof(tmp)-1);
		uncontracts[i]=new char[strlen(tmp)+1];
		strncpy(uncontracts[i],tmp,strlen(tmp)+1);
	}
	m_pUserApi->UnSubMarketData(uncontracts, unsubnum);
}

///用户退出应答
void CUSTPMDHandler::OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	printf("OnRspUserLogout:\n");
	printf("UserID:[%s] \n",pRspUserLogout->UserID);
	printf("ParticipantID:[%s] \n",pRspUserLogout->BrokerID);
	printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
}

// 深度行情通知，行情服务器会主动通知客户端
void CUSTPMDHandler::OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pMarketData) 
{
    // 客户端按需处理返回的数据

	fprintf(m_pFileOutput,"%s,%s,%d,",pMarketData->InstrumentID,pMarketData->UpdateTime,pMarketData->UpdateMillisec);
	if (pMarketData->AskPrice1==DBL_MAX)
		fprintf(m_pFileOutput,"%s,","");
	else
		fprintf(m_pFileOutput,"%f,",pMarketData->AskPrice1);

	if (pMarketData->BidPrice1==DBL_MAX)
		fprintf(m_pFileOutput,"%s \n","");
	else
		fprintf(m_pFileOutput,"%f \n",pMarketData->BidPrice1);
	fflush(m_pFileOutput);
}

// 针对用户请求的出错通知
void CUSTPMDHandler::OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    printf("OnRspError:\n");
    printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
    // 客户端需进行错误处理
}

///订阅合约的相关信息
void CUSTPMDHandler::OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	printf("Sub 返回订阅合约：%s \n",pSpecificInstrument->InstrumentID);
}

///订阅合约的相关信息
void CUSTPMDHandler::OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	printf("UnSub 返回订阅合约：%s \n",pSpecificInstrument->InstrumentID);
}
