// TestApi.cpp : Defines the entry point for the console application.
//
#include "PublicFuncs.h"
#include "TraderSpi.h"
#include "USTPFtdcTraderApi.h"
#include <iostream>

int g_nSwitch=0;

extern int g_nOrdSpeed;

void usage(char* pName)
{
	printf("usage:%s FrontAddress (tcp://172.31.112.133:15555)\n",pName);
	return;
}

int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		usage(argv[0]);
		return -1;
	}

	//init
	memset(g_frontaddr,0,BUFLEN);
	//strcpy(g_frontaddr,"tcp://172.31.112.133:15555");
	strcpy(g_frontaddr,argv[1]);
	
	printf("�����¼\n");
	printf("�������Ա�ţ�\n" );
	scanf("%s",g_BrokerID);
	printf("��Ա��Ϊ[%s]\n",g_BrokerID );
	printf("�������û��ţ�\n" );
	scanf("%s",g_UserID);
	printf("�û���Ϊ[%s]\n",g_UserID );
	printf("���������룺\n" );
	scanf("%s",g_Password);
	printf("����Ϊ[%s]\n",g_Password );
	printf("Input g_BrokerID=[%s]g_UserID=[%s]g_Password=[%s]\n",g_BrokerID,g_UserID,g_Password);
	getchar();
	CUstpFtdcTraderApi *pTrader = CUstpFtdcTraderApi::CreateFtdcTraderApi("");	
	g_puserapi=pTrader;

 	CTraderSpi spi(pTrader);
 	pTrader->RegisterFront(g_frontaddr);	
	pTrader->SubscribePrivateTopic(USTP_TERT_RESTART);
	pTrader->SubscribePublicTopic(USTP_TERT_RESTART);
	pTrader->RegisterSpi(&spi);
	pTrader->Init();

	pTrader->Join();
	pTrader->Release();


	return 0;
}
