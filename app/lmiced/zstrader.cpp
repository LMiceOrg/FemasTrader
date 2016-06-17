#include "lmicedtrader.h"

#include <stdio.h>
#include <string.h>

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
    LmicedTrader spi;//(pTrader);
    //init
    memset(spi.g_frontaddr,0,BUFLEN);
    //strcpy(g_frontaddr,"tcp://172.31.112.133:15555");
    strcpy(spi.g_frontaddr,argv[1]);

    printf("请求登录\n");
    printf("请输入会员号：\n" );
    scanf("%s",spi.g_BrokerID);
    printf("会员号为[%s]\n",spi.g_BrokerID );
    printf("请输入用户号：\n" );
    scanf("%s",spi.g_UserID);
    printf("用户号为[%s]\n",spi.g_UserID );
    printf("请输入密码：\n" );
    scanf("%s",spi.g_Password);
    printf("密码为[%s]\n",spi.g_Password );
    printf("Input g_BrokerID=[%s]g_UserID=[%s]g_Password=[%s]\n",spi.g_BrokerID,spi.g_UserID,spi.g_Password);
    CZeusingFtdcTraderApi *pTrader = CZeusingFtdcTraderApi::CreateFtdcTraderApi("");
    spi.m_pUserApi = pTrader;

    pTrader->RegisterFront(spi.g_frontaddr);
    pTrader->SubscribePrivateTopic(ZEUSING_TERT_RESTART);
    pTrader->SubscribePublicTopic(ZEUSING_TERT_RESTART);
    pTrader->RegisterSpi(&spi);
    pTrader->Init();

    pTrader->Join();
    pTrader->Release();

    return 0;
}

