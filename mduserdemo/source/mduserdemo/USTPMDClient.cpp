///@system	 飞马行情接受系统
///@company  上海金融期货信息技术有限公司
///@file	 CUstpMs
///@brief	 接受飞马系统的行情
///@history 
///20130502: 徐忠华 创建
//////////////////////////////////////////////////////////////////////

#include "USTPMDClient.h"

bool CUstpMs::InitInstance(char *  number,char * inifile)
{
	char tmp[200];
	strcpy(m_number,number);
//	CConfig *pConfig = 	new CConfig(INI_FILE_NAME);	
	TIniFile tf;	
	if (!tf.Open(inifile))
	{
		printf("不能打开配置文件\n");
		exit(-1000);
	}

	// 产生一个CFfexFtdcMduserApi实例
	pUserApi = CUstpFtdcMduserApi::CreateFtdcMduserApi();
	// 产生一个事件处理的实例
	sh = new CUSTPMDHandler(pUserApi,m_number);
	// 注册一事件处理的实例
	pUserApi->RegisterSpi(sh);
	// 注册需要的深度行情主题
	/// USTP_TERT_RESTART:从本交易日开始重传
	/// USTP_TERT_RESUME:从上次收到的续传
	/// USTP_TERT_QUICK:先传送当前行情快照,再传送登录后市场行情的内容

	int topicid = tf.ReadInt(m_number,"Topic",100);
	int tert = tf.ReadInt(m_number,"TERT",0);

	switch(tert)
	{
	case 0:
		{
			pUserApi->SubscribeMarketDataTopic(topicid, USTP_TERT_RESTART);
			break;
		}
	case 1:
		{
			pUserApi->SubscribeMarketDataTopic(topicid, USTP_TERT_RESUME);
			break;
		}
	case 2:
		{
			pUserApi->SubscribeMarketDataTopic(topicid, USTP_TERT_QUICK);
			break;
		}
	default:
		{
			printf("配置 TERT 值不对! \n");
			exit(-1);
		}
	}

	// 设置行情发布服务器的地址

	tf.ReadString(m_number,"MDFrontAdd","",tmp,sizeof(tmp)-1);
	mytrim(tmp);
	pUserApi->RegisterFront(tmp);
	// 使客户端开始与行情发布服务器建立连接
	int a=0;
	int b=0;
	printf(pUserApi->GetVersion(a,b));
//	pUserApi->SetHeartbeatTimeout(300);
	pUserApi->Init();
	// 等待行情接收
//	pUserApi->Join();
	// 释放useapi实例
//	pUserApi->Release();

	return true;
}

void CUstpMs::ExitInstance()
{
	pUserApi->Release();
	delete sh;
}
