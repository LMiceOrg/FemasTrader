#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include "lmice_trace.h"
#include "lmice_eal_bson.h"
#include "guava_demo.h"
#include "profile.h"
#include "fieldhelper.h"


using std::cout;
using std::cin;
using std::endl;

#ifdef DEMO_CACHE
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID

extern int g_log_pos;
extern int g_id_pos;
extern PERF_LOG g_perf[25000];


#else
extern int64_t g_time[][25000];
extern int g_b_pos;
extern int g_r_pos;
extern int g_s_pos;
#endif
#else
extern char g_msg[][1024];
#endif
extern int g_pos;
#endif


extern CUstpFtdcTraderApi *g_trader_api;
extern CTraderSpiT *g_thrader_spi;

void CTraderSpiT::OnFrontConnected()
{
	printf("OnFrontConnected\n");
	
	CUstpFtdcReqUserLoginField req;
	memset(&req, 0, sizeof req);
	strcpy(req.BrokerID, "0129");
	strcpy(req.UserID, "012981073650");
	strcpy(req.Password, "123456");
	m_ptr_api->ReqUserLogin(&req, 15);
}

void CTraderSpiT::OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    printf("OnRspError:\n");
    printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
}

void CTraderSpiT::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo!=NULL&&pRspInfo->ErrorID!=0)
	{
		printf("-----------------------------\n");
		printf("登录失败...错误原因：%s\n",pRspInfo->ErrorMsg );
		printf("ErrorID=%d\n", pRspInfo->ErrorID);
		printf("-----------------------------\n");

		return;
	}

	
	int64_t systime = 0;
	get_system_time(&systime);

	m_MaxOrderLocalID=atoi(pRspUserLogin->MaxOrderLocalID)+1;
	printf("-----------------------------\n");
	printf("登录成功，最大本地报单号:%d, 登录时间:%lu\n",m_MaxOrderLocalID, systime/10);
	printf("-----------------------------\n");

/*
	{
		int m_last_px = 0;
		
		
		CUstpFtdcInputOrderField *orderTest = new CUstpFtdcInputOrderField;
		memset( orderTest, 0, sizeof(CUstpFtdcInputOrderField) );

		strcpy(orderTest->BrokerID, "0177");
		strcpy(orderTest->UserID, "56220000880301");
		strcpy(orderTest->ExchangeID, "SHFE");
		strcpy(orderTest->InstrumentID, "rb1610");
		strcpy(orderTest->InvestorID, "5622000088");
		orderTest->OrderPriceType = USTP_FTDC_OPT_LimitPrice;
		orderTest->Direction = USTP_FTDC_D_Buy;
		orderTest->OffsetFlag = USTP_FTDC_OF_Open;
		orderTest->HedgeFlag = USTP_FTDC_CHF_Speculation;
		orderTest->LimitPrice = m_last_px - 10;
		orderTest->Volume = 1;
		orderTest->TimeCondition = USTP_FTDC_TC_IOC;
		orderTest->VolumeCondition = USTP_FTDC_VC_AV;
		orderTest->ForceCloseReason = USTP_FTDC_FCR_NotForceClose;
		int local_id = GetLocalID();

		char tmp[13];
		memset(tmp, 0, 13);
		sprintf(tmp, "%012d", local_id);
		strcpy(orderTest->UserOrderLocalID, tmp);
		
		int64_t systime = 0;
		get_system_time(&systime);
		m_ptr_api->ReqOrderInsert( orderTest, local_id );

		EalBson bson;
		bson.AppendInt64("time", systime/10);
		bson.AppendSymbol("BrokerID", orderTest->BrokerID);
		bson.AppendSymbol("ExchangeID", orderTest->ExchangeID);
		bson.AppendSymbol("BrokerID", orderTest->BrokerID);
		bson.AppendSymbol("InstrumentID", orderTest->InstrumentID);
		bson.AppendSymbol("InvestorID", orderTest->InvestorID);
		bson.AppendFlag("OrderPriceType", orderTest->OrderPriceType);
		bson.AppendFlag("Direction", orderTest->Direction);
		bson.AppendFlag("OffsetFlag", orderTest->OffsetFlag);
		bson.AppendFlag("HedgeFlag", orderTest->HedgeFlag);
		bson.AppendDouble("LimitPrice", orderTest->LimitPrice);
		bson.AppendInt32("Volume", orderTest->Volume);
		bson.AppendFlag("TimeCondition", orderTest->TimeCondition);
		bson.AppendFlag("VolumeCondition", orderTest->VolumeCondition);
		bson.AppendFlag("ForceCloseReason", orderTest->ForceCloseReason);
		bson.AppendSymbol("UserOrderLocalID", orderTest->UserOrderLocalID);
		
		const char *strJson = bson.GetJsonData();
		m_time_log[0] = systime/10;
		sprintf( m_opt_log[0], "send order inster:",strJson );
		bson.FreeJsonData();
		
	}
	

CUstpFtdcQryInstrumentField QryInstrument;
memset( &QryInstrument, 0, sizeof(CUstpFtdcQryInstrumentField));
strcpy( QryInstrument.ExchangeID, "SHFE" );

int64_t systime = 0;
get_system_time(&systime);
EalBson bson;
bson.AppendInt64("time", systime/10);

m_ptr_api->ReqQryInstrument( &QryInstrument , 1 );
const char *strJson = bson.GetJsonData();
printf("[future pkt] send ( Instrument )\ncontent: %s\n\n", strJson);
bson.FreeJsonData();

sleep(10);

CUstpFtdcQryUserInvestorField UserInvestor;
memset(&UserInvestor, 0, sizeof(CUstpFtdcQryUserInvestorField));

strcpy(UserInvestor.BrokerID, "0177");
strcpy(UserInvestor.UserID, "56220000880301");


get_system_time(&systime);
EalBson bson2;
bson2.AppendInt64("time", systime);
strJson = bson2.GetJsonData();

m_ptr_api->ReqQryUserInvestor( &UserInvestor, 2);
printf("[future pkt] send ( UserInvestor )\ncontent: %s\n\n", strJson);
bson2.FreeJsonData();
*/
}

void CTraderSpiT::OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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
	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	bson.AppendInt64("time", systime/10);
	bson.AppendSymbol("ExchangeID", pRspInstrument->ExchangeID);
	bson.AppendSymbol("ProductID", pRspInstrument->ProductID);
	bson.AppendSymbol("ProductName", pRspInstrument->ProductName);
	bson.AppendSymbol("InstrumentID", pRspInstrument->InstrumentID);
	bson.AppendSymbol("InstrumentName", pRspInstrument->InstrumentName);

	const char *strJson = bson.GetJsonData();
	printf("[future pkt] recv ( Instrument )\ncontent: %s\n\n", strJson);
	//m_logging.logging("[future pkt] recv ( normal ), content: %s", strJson);

	bson.FreeJsonData();
}


void CTraderSpiT::OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pUserInvestor, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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

	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	bson.AppendInt64("time", systime/10);
	bson.AppendSymbol("BrokerID", pUserInvestor->BrokerID);
	bson.AppendSymbol("UserID", pUserInvestor->UserID);
	bson.AppendSymbol("InvestorID", pUserInvestor->InvestorID);

	const char *strJson = bson.GetJsonData();
	printf("[future pkt] recv ( Instrument )\ncontent: %s\n\n", strJson);
	//m_logging.logging("[future pkt] recv ( normal ), content: %s", strJson);

	bson.FreeJsonData();
	
}


void CTraderSpiT::OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInputOrder==NULL )
    {
        printf("没有查询到合约数据\n");
        return ;
    }

	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	bson.AppendInt64("recv time", systime/10);
	bson.AppendSymbol("OrderSysID", pInputOrder->OrderSysID);
	bson.AppendSymbol("UserOrderLocalID",pInputOrder->UserOrderLocalID);
	if( pRspInfo != NULL )
	{
		bson.AppendInt64("id", pRspInfo->ErrorID);
		bson.AppendSymbol("reason", gbktoutf8(pRspInfo->ErrorMsg).c_str());
		const char *strJson = bson.GetJsonData();
#ifdef DEMO_CACHE
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID
#else
	g_time[2][g_s_pos++] = systime/10;
#endif
#else
	sprintf( g_msg[g_pos++], "[future pkt] recv ( OnRspOrderInsert )\ncontent: %s\n\n", strJson);
#endif
#else
	printf( "[future pkt] recv ( OnRspOrderInsert )\ncontent: %s\n\n", strJson);
#endif
		bson.FreeJsonData();
		return;
	}

	//const char *strJson = bson.GetJsonData();
#ifdef DEMO_CACHE
	//sprintf( g_msg[g_pos++], "[future pkt] recv ( OnRspOrderInsert )\ncontent: %s\n\n", strJson);
#else
	//printf( "[future pkt] recv ( OnRspOrderInsert )\ncontent: %s\n\n", strJson);
#endif

	//bson.FreeJsonData();
	return;

	
}

void CTraderSpiT::OnRtnOrder( CUstpFtdcOrderField *pOrder )
{	
	if (pOrder==NULL)
    {
        printf("没有查询到合约数据\n");
        return ;
    }
	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	bson.AppendInt64("recv time", systime/10);
	bson.AppendSymbol("OrderSysID", pOrder->OrderSysID);
	bson.AppendSymbol("UserOrderLocalID",pOrder->UserOrderLocalID);
	const char *strJson = bson.GetJsonData();

#ifdef DEMO_CACHE
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID
	strcpy(g_perf[g_id_pos++].sys_id,pOrder->OrderSysID);
        printf(" %s - %s\n", g_perf[g_id_pos-1].test_time, g_perf[g_id_pos-1].sys_id);
#else
	g_time[1][g_r_pos++] = systime/10;
#endif
#else
	sprintf( g_msg[g_pos++], "[future pkt] recv ( OnRtnOrder )\ncontent: %s\n\n", strJson);
#endif
#else
	printf( "[future pkt] recv ( OnRtnOrder )\ncontent: %s\n\n", strJson);
#endif

	bson.FreeJsonData();

}

void CTraderSpiT::OnRtnTrade(CUstpFtdcTradeField *pTrade) 
{
	if (pTrade==NULL)
    {
        printf("没有查询到合约数据\n");
        return ;
    }
	int64_t systime = 0;
	get_system_time(&systime);
	EalBson bson;
	bson.AppendInt64("recv time", systime/10);
	const char *strJson = bson.GetJsonData();
	printf( "[future pkt] recv ( OnRtnTrade )\ncontent: %s\n\n", strJson);
	bson.FreeJsonData();
}


int CTraderSpiT::code_convert(const char *from_charset,const char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
    iconv_t cd;
    int rc;
    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset,from_charset);
    if (cd==0)
            return -1;
    memset(outbuf,0,outlen);
    if (iconv(cd,pin,&inlen,pout,&outlen) == -1)
            return -1;
    iconv_close(cd);
    return 0;
}

string CTraderSpiT::gbktoutf8( char *pgbk)
{
    size_t inlen = strlen(pgbk);
    char outbuf[512] ={0};
    size_t outlen = 512;
    code_convert("gb2312","utf-8",pgbk,inlen,(char*)outbuf,outlen);

    std::string utf8;
    utf8.insert(utf8.begin(), outbuf, outbuf+strlen(outbuf));
    return utf8;

}

void CTraderSpiT::OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo) 
{
	if (pRspInfo!=NULL)
	{
		int64_t systime = 0;
		get_system_time(&systime);
		EalBson bson;
		bson.AppendInt64("recv time", systime/10);
		bson.AppendInt64("id", pRspInfo->ErrorID);
		bson.AppendSymbol("reason", gbktoutf8(pRspInfo->ErrorMsg).c_str());
		bson.AppendSymbol("OrderSysID", pInputOrder->OrderSysID);
		bson.AppendSymbol("UserOrderLocalID", pInputOrder->UserOrderLocalID);
		const char *strJson = bson.GetJsonData();
		printf("[future pkt] recv ( OnErrRtnOrderInsert )\ncontent: %s\n\n", strJson);
		bson.FreeJsonData();
		return;
	}
	if (pInputOrder==NULL)
	{
		printf("No data\n");
		return ;
	}

}



guava_demo::guava_demo( CTraderSpiT *pSpi )
{
    m_pFileOutput=NULL;
	m_trader_spi = pSpi;
	m_quit_flag = false;
	m_trader_api = pSpi->GetApi();
	m_flag = 0; 
	m_last_price = 0;
	
	string type = "hc_0";
	string date = "";
	m_forecaster = ForecasterFactory::createForecaster( type, date );
}

guava_demo::~guava_demo(void)
{

}

void guava_demo::run()
{
	input_param();

	bool ret = init();
	if (!ret)
	{
		string str_temp;

		printf("\n input any char to exit:\n");
		cin >> str_temp;

		return;
	}

	while(!m_quit_flag)
	{
		pause();
	}

#ifdef DEMO_CACHE
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID
/*
	int i=0;
	printf("stastic:");
	printf( "total order: %d\n total rtn: %d\n", g_log_pos, g_id_pos);
	printf("\t\torder\t\t\tsysid\n");
	for( ; i<g_log_pos; i++ )
	{
		printf("\t\t%s\t  %s\n", g_perf[i].test_time, g_perf[i].sys_id);
	}
*/
#else
	int i = 0;
	printf("order: ");	
	printf("total order: %d\n total order input rsponse: %d\n total order return: %d\n", g_b_pos, g_s_pos, g_r_pos);
	printf("\t\torder\t\t\trsp\t\tinterval\t\trtn\t\tinterval \n");
//	printf("==================== round 1 ===================\n");
	for( int i=0; i<g_b_pos; i++ )
	{
		printf("%24llu", g_time[0][i]);
		printf("%24llu", g_time[2][i]);
		int sub = g_time[2][i] - g_time[0][i];
		printf("%16d", sub);
		printf("%24llu", g_time[1][i]);
		sub = g_time[1][i] - g_time[0][i];
		printf("%16d\n", sub);
//		if((i+1)%10 == 0 && i<(g_b_pos-1))
//			printf("==================== round %d  ===================\n", (i+1)/10+1);
	}
#endif
#else
	for( int i=0; i<g_pos; i++ )
	{
		if( strlen(g_msg[i]) > 0 )
		{
			printf( "%s\n", g_msg[i] );
		}
	}
#endif
#endif
	close();
}

void guava_demo::input_param()
{
    string local_ip = "10.10.21.191";
	string cffex_ip = "238.0.1.2";
	int cffex_port = 5001;
	string cffex_local_ip = local_ip;
	int cffex_local_port = 23501;

	multicast_info temp;

	memset(&temp, 0, sizeof(multicast_info));
	
	strcpy(temp.m_remote_ip, cffex_ip.c_str());
	temp.m_remote_port = cffex_port;
	strcpy(temp.m_local_ip, cffex_local_ip.c_str());
	temp.m_local_port = cffex_local_port;

	m_cffex_info = temp;
}

bool guava_demo::init()
{
	return m_guava.init(m_cffex_info, this);
}

void guava_demo::close()
{
	m_guava.close();
}

void guava_demo::pause()
{
	string str_temp;

	printf("\n按任意字符继续(输入q将退出):\n");
	cin >> str_temp;
	if (str_temp == "q")
	{
		m_quit_flag = true;
	}
}

static void show_order(CUstpFtdcInputOrderField *order)
{
			int64_t systime = 0;
			get_system_time(&systime);
			EalBson bson;
			
			bson.AppendInt64("time", systime/10);
			bson.AppendSymbol("BrokerID", order->BrokerID);
			bson.AppendSymbol("ExchangeID", order->ExchangeID);
			bson.AppendSymbol("BrokerID", order->BrokerID);
			bson.AppendSymbol("InstrumentID", order->InstrumentID);
			bson.AppendSymbol("InvestorID", order->InvestorID);
			bson.AppendFlag("OrderPriceType", order->OrderPriceType);
			bson.AppendFlag("Direction", order->Direction);
			bson.AppendFlag("OffsetFlag", order->OffsetFlag);
			bson.AppendFlag("HedgeFlag", order->HedgeFlag);
			bson.AppendDouble("LimitPrice", order->LimitPrice);
			bson.AppendInt32("Volume", order->Volume);
			bson.AppendFlag("TimeCondition", order->TimeCondition);
			bson.AppendFlag("VolumeCondition", order->VolumeCondition);
			bson.AppendFlag("ForceCloseReason", order->ForceCloseReason);
			bson.AppendSymbol("UserOrderLocalID", order->UserOrderLocalID);
	
			const char *strJson = bson.GetJsonData();
	
#ifdef DEMO_CACHE		
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID
#else
			g_time[0][g_b_pos++] = systime/10;
#endif
#else
			sprintf( g_msg[g_pos++], "[future opt] send ( ReqOrderInsert )\ncontent: %s\n\n", strJson);
#endif
#else
			printf( "[future opt] send ( ReqOrderInsert )\ncontent: %s\n\n", strJson );
#endif
			bson.FreeJsonData();	

}


void * guava_demo::order_thread(void *ptrArg)
{

	char tmp[13];
	memset(tmp, 0, 13);
	int local_id = 0;

	CUstpFtdcInputOrderField *orderTest = (CUstpFtdcInputOrderField *) ptrArg;

	int round = 1;
	int interval = 500/round; 
	interval = 0;
	
	for( int i=0; i<round; i++ )
	{
		local_id = g_thrader_spi->GetLocalID();
		sprintf(tmp, "%012d", local_id);
		strcpy(orderTest->UserOrderLocalID, tmp);
		show_order(orderTest);
		g_trader_api->ReqOrderInsert( orderTest, local_id );
		if( interval > 0 )
			usleep(interval*1000);
	}
}

void guava_demo::on_receive_data( pIncQuotaDataT ptrData )
{
	if( NULL == ptrData )
	{
		lmice_error_print("receive md data error\n");
		return;
	}

	//if( m_flag >= 0 && ptrData->InstTime.UpdateMillisec == 0)
	{
		//m_flag--;

		ChinaL1Msg dataMsg;


		/*
		CUstpFtdcInputOrderField *orderTest = new CUstpFtdcInputOrderField;
		memset( orderTest, 0, sizeof(CUstpFtdcInputOrderField) );

		if( ptrData->PriVol.LastPrice > 0 )
		{
			m_last_price = ptrData->PriVol.LastPrice;
		}

		#ifdef DEMO_CACHE_SYSID
		char *tmp = (char *)ptrData->InstTime.UpdateTime;
		strcpy(g_perf[g_log_pos++].test_time, tmp);
		#endif
		
		strcpy(orderTest->BrokerID, "0129");
		strcpy(orderTest->UserID, "012981073650");
		strcpy(orderTest->ExchangeID, "SHFE");
		strcpy(orderTest->InstrumentID, CUR_INS1);
		strcpy(orderTest->InvestorID, "81073650");
		orderTest->OrderPriceType = USTP_FTDC_OPT_LimitPrice;
		orderTest->Direction = USTP_FTDC_D_Buy;
		orderTest->OffsetFlag = USTP_FTDC_OF_Open;
		orderTest->HedgeFlag = USTP_FTDC_CHF_Speculation;
		orderTest->LimitPrice = m_last_price - 20;
		orderTest->Volume = 1;
		orderTest->TimeCondition = USTP_FTDC_TC_IOC;
		orderTest->VolumeCondition = USTP_FTDC_VC_AV;
		orderTest->ForceCloseReason = USTP_FTDC_FCR_NotForceClose;

		
	
		pthread_t thread_id;
		pthread_attr_t thread_attr;
		pthread_attr_init(&thread_attr);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED); 
		int ret = pthread_create(&thread_id, &thread_attr, order_thread, (void *)orderTest);
	*/	
	}
}


/************************************************************************/
/* function: get the current local timestamp
*  author: lgz
*  create date: 2016.4.29
*/
/************************************************************************/
void gettimestamp(char timestamp[13])
{
	struct  timeval    tv;

	struct  timezone   tz;
	long tv_usec;

	gettimeofday(&tv, &tz);

	tv_usec = tv.tv_usec;

	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	sprintf(timestamp, "%.2d:%.2d:%.2d:%ld",
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tv_usec);
}

