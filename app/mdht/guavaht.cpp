#include "guava_demo.h"
#include "USTPFtdcTraderApi.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>


using namespace std;

#ifdef DEMO_CACHE
int  g_pos = 0;
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID
int g_id_pos = 0;
int g_log_pos = 0;
PERF_LOG g_perf[25000];
#else
int g_b_pos = 0;
int g_s_pos = 0;
int g_r_pos = 0;
int64_t g_time[3][25000];
#endif

#else
char g_msg[25000][1024];

static void msg_log( char *strLog )
{
	strcpy( g_msg[g_pos++], strLog );
}

#endif
#endif

CUstpFtdcTraderApi *g_trader_api = NULL;
CTraderSpiT *g_thrader_spi = NULL;

int main()
{
	CUstpFtdcTraderApi *pTraderApi = CUstpFtdcTraderApi::CreateFtdcTraderApi("");
	CTraderSpiT *pTraderSpi = new CTraderSpiT( pTraderApi );
	pTraderApi->RegisterSpi( pTraderSpi );
	pTraderApi->RegisterFront("tcp://10.10.21.52:8002");
	pTraderApi->SubscribePrivateTopic(USTP_TERT_QUICK);
	pTraderApi->SubscribePublicTopic(USTP_TERT_QUICK);
	pTraderApi->Init();
	g_trader_api = pTraderApi;
	g_thrader_spi = pTraderSpi;

#ifdef DEMO_CACHE
#ifdef DEMO_CACHE_ONLY_TIME
#ifdef DEMO_CACHE_SYSID
	for(int i=0; i<25000; i++)
	{
		memset(g_perf[i].test_time, 0, 1024);
		memset(g_perf[i].sys_id, 0, 32);
	}

#else
	for( int i=0; i<25000; i++ )
	{
		g_time[0][i] = 0;
		g_time[1][i] = 0;
		g_time[2][i] = 0;
	}
#endif
#else
	for( int j=0; j<25000; j++ )
	{
		memset(g_msg[j], 0, 1024);
	}
#endif
#endif

//	while(1)
//{
//	sleep(1);
//}

	guava_demo temp( pTraderSpi );
	temp.run();
	return 0;
}









