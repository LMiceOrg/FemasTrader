///@system	 �����������ϵͳ
///@company  �Ϻ������ڻ���Ϣ�������޹�˾
///@file	 CUstpMDClient
///@brief	 ���ܷ���ϵͳ������
///@history 
///20130502: ���һ� ����
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
	//����ϵͳ��ʼ��
	virtual bool InitInstance(char *  number,char * inifile);
	//����ϵͳ�˳�
	virtual void ExitInstance();
	CUstpFtdcMduserApi *pUserApi;

private:
	CUSTPMDHandler *sh;
	//���еĶ��ĺ�
	char m_number[256];
};

#endif