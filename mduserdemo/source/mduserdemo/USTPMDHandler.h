#ifndef CUSTPMDHandler_H_
#define CUSTPMDHandler_H_

#include "USTPFtdcMduserApi.h"
#include "profile.h"

class CUSTPMDHandler : public CUstpFtdcMduserSpi
{
public:
    // ���캯������Ҫһ����Ч��ָ��CUstpFtdcMduserApiʵ����ָ��
    CUSTPMDHandler(CUstpFtdcMduserApi *pUserApi,char * number);
    ~CUSTPMDHandler();
    // ���ͻ��������鷢��������������ͨ�����ӣ��ͻ�����Ҫ���е�¼
    void OnFrontConnected();

    // ���ͻ��������鷢��������ͨ�����ӶϿ�ʱ���÷���������
    void OnFrontDisconnected();

	void OnRspQryTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // ���ͻ��˷�����¼����֮�󣬸÷����ᱻ���ã�֪ͨ�ͻ��˵�¼�Ƿ�ɹ�
    void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�û��˳�Ӧ��
	void OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// �������֪ͨ�����������������֪ͨ�ͻ���
    void OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pMarketData);
  
	// ����û�����ĳ���֪ͨ
    void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///���ĺ�Լ�������Ϣ
	virtual void OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///���ĺ�Լ�������Ϣ
	virtual void OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
    // ָ��CUstpFtdcMduserApiʵ����ָ��
    CUstpFtdcMduserApi *m_pUserApi;
	//���ÿ�δ�ӡ�Ļ���
	char buf[2048];
	//Ҫд����ļ�
	FILE *m_pFileOutput;
	TIniFile tf;
	//�������
	char m_number[256];
	char tmp[256];
};
#endif