///@system	 中金所行情源
///@company  上海金融期货信息技术有限公司
///@file	 CCffexMs
///@brief	 接受中金所的行情，保存并提供客户端的介入
///@history 
///20121119: 徐忠华 创建
//////////////////////////////////////////////////////////////////////

#include "profile.h"
#include "USTPMDClient.h"

char *INI_FILE_NAME = "mduserdemo.ini";

int main()
{
	TIniFile tf;
	char tmp[256];	
	if (!tf.Open(INI_FILE_NAME))
	{
		printf("不能打开配置文件\n");
		exit(-1000);
	}
	int marketnum = tf.ReadInt("COMMON","MARKETDATANUM",1);
	int i = 0;
	for(;i<marketnum;i++)
	{
		memset(tmp, 0, 256);
		sprintf(tmp,"MARKETDATA%d",i+1);
		printf("init instance %s\n", tmp);
		CUstpMs ustpMs;
		ustpMs.InitInstance(tmp,INI_FILE_NAME);
	}
	printf("\ngo looping...\n");
	while(true)
	{
		SLEEP_SECONDS(5000);
	}
	return 0;
}
