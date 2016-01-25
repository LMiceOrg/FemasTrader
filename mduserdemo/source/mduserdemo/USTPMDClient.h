///@system	 飞马行情接受系统
///@company  上海金融期货信息技术有限公司
///@file	 CUstpMDClient
///@brief	 接受飞马系统的行情
///@history 
///20130502: 徐忠华 创建
//////////////////////////////////////////////////////////////////////

#ifndef CUSTPClient_H_
#define CUSTPClient_H_


#include "USTPMDHandler.h"
#include "USTPFtdcMduserApi.h"
#include "profile.h"

class CUstpMs
{
public:
	CUstpMs(){}
	~CUstpMs(){}
	//行情系统初始化
	virtual bool InitInstance(char *  number,char * inifile);
	//行情系统退出
	virtual void ExitInstance();
	CUstpFtdcMduserApi *pUserApi;

private:
	CUSTPMDHandler *sh;
	//运行的订阅号
	char m_number[256];
};

#endif