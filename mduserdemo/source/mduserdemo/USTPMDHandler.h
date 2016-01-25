#ifndef CUSTPMDHandler_H_
#define CUSTPMDHandler_H_

#include "USTPFtdcMduserApi.h"
#include "profile.h"

class CUSTPMDHandler : public CUstpFtdcMduserSpi
{
public:
    // 构造函数，需要一个有效的指向CUstpFtdcMduserApi实例的指针
    CUSTPMDHandler(CUstpFtdcMduserApi *pUserApi,char * number);
    ~CUSTPMDHandler();
    // 当客户端与行情发布服务器建立起通信连接，客户端需要进行登录
    void OnFrontConnected();

    // 当客户端与行情发布服务器通信连接断开时，该方法被调用
    void OnFrontDisconnected();

	void OnRspQryTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
    void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///用户退出应答
	void OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// 深度行情通知，行情服务器会主动通知客户端
    void OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pMarketData);
  
	// 针对用户请求的出错通知
    void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///订阅合约的相关信息
	virtual void OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///订阅合约的相关信息
	virtual void OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
    // 指向CUstpFtdcMduserApi实例的指针
    CUstpFtdcMduserApi *m_pUserApi;
	//存放每次打印的缓冲
	char buf[2048];
	//要写入的文件
	FILE *m_pFileOutput;
	TIniFile tf;
	//运行序号
	char m_number[256];
	char tmp[256];
};
#endif