// PublicFuncs.cpp: implementation of the PublicFuncs class.
//
//////////////////////////////////////////////////////////////////////

#include "PublicFuncs.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PublicFuncs::PublicFuncs()
{

}

PublicFuncs::~PublicFuncs()
{

}

typedef struct 
{
	int nordspeed;
	int nordloop;
} stordspeed;

/*---------------ȫ�ֱ�����---------------*/

const char* TEST_API_INI_NAME="./config/TestApi.ini";
char* g_pFlowPath="./flow/";
char* g_pProductInfo="��ʾ���Թ���V1.0";
char* g_pProgramName="TestApi";
int g_choose;

extern int g_nSwitch;
extern FILE * g_fpSend;

//1.��¼��Ϣ
///���͹�˾����
TUstpFtdcBrokerIDType g_BrokerID;
///�����û�����
TUstpFtdcUserIDType	g_UserID;
///����
TUstpFtdcPasswordType	g_Password;

//2.��ַ��Ϣ
char g_frontaddr[BUFLEN];

//3.������Ϣ
int g_nOrdLocalID=0;
CUstpFtdcTraderApi * g_puserapi=NULL;

/*---------------ȫ�ֺ�����---------------*/
void ShowManu()
{
	printf("**********************\n"
	       "ѡ����Ҫִ�еĲ���\n"
	       "1-�����걨\n"
	       "2-��������\n"
	       "3-������ѯ\n"
	       "4-�ɽ���ѯ\n"
	       "5-Ͷ�����˻���ѯ\n"
	       "6-���ױ����ѯ\n"
	       "7-��������ѯ\n"
	       "8-��Լ��Ϣ��ѯ\n"
	       "9-����Ͷ���߲�ѯ\n"
	       "10-�ͻ��ֲֲ�ѯ\n"
	       "11-�޸��û�����\n"
		   "12-Ͷ�����������ʲ�ѯ\n"
		   "13-Ͷ���߱�֤���ʲ�ѯ\n"
		   "14-�Ϲ������ѯ\n"
	       "0-�˳�\n"
	       "**********************\n"
	       "��ѡ��"
	);
	scanf("%d",&g_choose);
	
}


void StartInputOrder ()
{
	if (g_puserapi==NULL)
	{
		printf("StartInputOrder  USERAPIδ����");
		return ;
	}
	//CUstpFtdcInputOrderField* pord=g_pOrd->at(1);
	CUstpFtdcInputOrderField ord;
	memset(&ord,0,sizeof(CUstpFtdcInputOrderField));
	printf("�����뱨����Ϣ\n");
	
	printf("input InvestorID\n");
	scanf("%s",(ord.InvestorID));
	printf("InvestorID=[%s]\n",ord.InvestorID);
	
	printf("input InstrumentID\n");
	scanf("%s",(ord.InstrumentID));
	printf("InstrumentID=[%s]\n",(ord.InstrumentID));
	getchar();
	printf("input OrderPriceType ����[1:�м� 2:�޼�]����\n");
	scanf("%c",&(ord.OrderPriceType));
	printf("OrderPriceType=[%c]\n",(ord.OrderPriceType));
	getchar();
	printf("input Direction ������[0:�� 1:��]����\n");
	scanf("%c",&(ord.Direction));
	printf("Direction=[%c]\n",(ord.Direction));
	getchar();
	printf("input OffsetFlag ����[0:���� 1:ƽ��]����\n");
	scanf("%c",&(ord.OffsetFlag));
	printf("OffsetFlag=[%c]\n",(ord.OffsetFlag));
	getchar();
	printf("input HedgeFlag ����[1:Ͷ�� 2:�ױ� 3:����]����\n");
	scanf("%c",&(ord.HedgeFlag));
	printf("HedgeFlag=[%c]\n",(ord.HedgeFlag));
	getchar();
	printf("input LimitPrice\n");
	scanf("%lf",&(ord.LimitPrice));
	printf("LimitPrice=[%lf]\n",(ord.LimitPrice));
	
	printf("input Volume\n");
	scanf("%d",&(ord.Volume));
	printf("Volume=[%d]\n",(ord.Volume));
	getchar();
	printf("input TimeCondition ����[1:IOC  2:������Ч 3:������Ч]����\n");
	scanf("%c",&(ord.TimeCondition));
	printf("InstrumentID=[%c]\n",(ord.TimeCondition));	

	
	strcpy(ord.BrokerID,g_BrokerID);
	strcpy(ord.UserID,g_UserID);
	strcpy(ord.ExchangeID,"CFFEX");
	ord.VolumeCondition='1';
	ord.ForceCloseReason='0';
	sprintf(ord.UserOrderLocalID,"%012d",g_nOrdLocalID++);
		
	g_puserapi->ReqOrderInsert(&ord,g_nOrdLocalID++);


	return ;
}


void StartOrderAction()
{
	/*֧��ϵͳ�����źͱ��ر��������ֳ�����ʽ*/
	//-----------------------ϵͳ�����ų���----------------------------//
	int SysID;
	CUstpFtdcOrderActionField OrderAction;
	memset(&OrderAction,0,sizeof(CUstpFtdcOrderActionField));
	if (g_puserapi==NULL)
	{
		printf("StartOrderAction  USERAPIδ����");
		return ;
	}

	strcpy(OrderAction.ExchangeID,"CFFEX");
	strcpy(OrderAction.BrokerID,g_BrokerID);
	strcpy(OrderAction.UserID,g_UserID);

	printf("input InvestorID\n");
	scanf("%s",(OrderAction.InvestorID));
	printf("InvestorID=[%s]\n",OrderAction.InvestorID);
	
	printf("������ϵͳ�����ţ�");
	scanf("%d",&SysID);
	sprintf(OrderAction.OrderSysID,"%12d",SysID);
	printf("����ϵͳ������[%s]\n",OrderAction.OrderSysID);
	strcpy(OrderAction.UserOrderLocalID,"");
	OrderAction.ActionFlag=USTP_FTDC_AF_Delete;
	sprintf(OrderAction.UserOrderActionLocalID,"%012d",g_nOrdLocalID++);

	g_puserapi->ReqOrderAction(&OrderAction,g_nOrdLocalID++);
	
	return ;
	//-----------------------ϵͳ�����ų���----------------------------//

 
	//-----------------------���ر����ų���----------------------------//
    /*
         int LocalID;
         CUstpFtdcOrderActionField OrderAction;
         memset(&OrderAction,0,sizeof(CUstpFtdcOrderActionField));
         if (g_puserapi==NULL)
         {
                   printf("StartOrderAction  USERAPIδ����");
                   return ;
         }
         strcpy(OrderAction.ExchangeID,"CFFEX");
         strcpy(OrderAction.BrokerID,g_BrokerID);
         strcpy(OrderAction.UserID,g_UserID);
		 
		 printf("input InvestorID\n");
		 scanf("%s",(OrderAction.InvestorID));
		 printf("InvestorID=[%s]\n",OrderAction.InvestorID);
         
         printf("�����뱾�ر�����:");
         scanf("%d",&LocalID);
         sprintf(OrderAction.UserOrderLocalID,"%012d",LocalID);

         printf("�������ر�����[%s]\n",OrderAction.UserOrderLocalID );
         strcpy(OrderAction.OrderSysID,"");
         
         OrderAction.ActionFlag=USTP_FTDC_AF_Delete;
         sprintf(OrderAction.UserOrderActionLocalID,"%012d",g_nOrdLocalID++);
         
         g_puserapi->ReqOrderAction(&OrderAction,g_nOrdLocalID++);
         
         return ;
    */
	//-----------------------���ر����ų���----------------------------//

}

void StartQueryExchange()
{
	CUstpFtdcQryExchangeField QryExchange;
	if (g_puserapi==NULL)
	{
		printf("StartQueryExchange  USERAPIδ����");
		return ;
	}

	strcpy(QryExchange.ExchangeID,"CFFEX");
	g_puserapi->ReqQryExchange(&QryExchange,g_nOrdLocalID++);
	return ;
}

void StartQryTrade()
{
	CUstpFtdcQryTradeField QryTrade;
	memset(&QryTrade,0,sizeof(CUstpFtdcQryTradeField));
	if (g_puserapi==NULL)
	{
		printf("StartQryTrade  USERAPIδ����");
		return ;

	}
	strcpy(QryTrade.ExchangeID,"CFFEX");
	strcpy(QryTrade.BrokerID,g_BrokerID);
	strcpy(QryTrade.UserID,g_UserID);
	//strcpy(QryTrade.TradeID,"");
	g_puserapi->ReqQryTrade(&QryTrade,g_nOrdLocalID++);
	return ;
}
void StartQryOrder()
{
	CUstpFtdcQryOrderField QryOrder;
	memset(&QryOrder,0,sizeof(CUstpFtdcQryOrderField));
	if (g_puserapi==NULL)
	{
		printf("StartQryOrder  USERAPIδ����");
		return ;

	}
	strcpy(QryOrder.ExchangeID,"CFFEX");
	strcpy(QryOrder.BrokerID,g_BrokerID);
	strcpy(QryOrder.UserID,g_UserID);
	//strcpy(QryOrder.OrderSysID,"");
	g_puserapi->ReqQryOrder(&QryOrder,g_nOrdLocalID++);
	return ;
}

void StartQryInvestorAccount()
{
	CUstpFtdcQryInvestorAccountField QryInvestorAcc;
	if (g_puserapi==NULL)
	{
		printf("StartQryInvestorAccount  USERAPIδ����");
		return ;

	}
	memset(&QryInvestorAcc,0,sizeof(CUstpFtdcQryInvestorAccountField));
	strcpy(QryInvestorAcc.BrokerID,g_BrokerID);
//	strcpy(QryInvestorAcc.InvestorID,"8");
	g_puserapi->ReqQryInvestorAccount(&QryInvestorAcc,g_nOrdLocalID++);

	return ;
}

void StartQryUserInvestor()
{
	CUstpFtdcQryUserInvestorField QryUserInvestor;
	if (g_puserapi==NULL)
	{
		printf("StartQryInvestorAccount  USERAPIδ����");
		return ;

	}
	memset(&QryUserInvestor,0,sizeof(CUstpFtdcQryUserInvestorField));
	strcpy(QryUserInvestor.BrokerID,g_BrokerID);
	strcpy(QryUserInvestor.UserID,g_UserID);
	g_puserapi->ReqQryUserInvestor(&QryUserInvestor,g_nOrdLocalID++);
	return ;
}

void StartQryInstrument()
{
	CUstpFtdcQryInstrumentField QryInstrument;
	if (g_puserapi==NULL)
	{
		printf("StartQryInvestorAccount  USERAPIδ����");
		return ;

	}
	memset(&QryInstrument,0,sizeof(CUstpFtdcQryInstrumentField));
	strcpy(QryInstrument.ExchangeID,"CFFEX");
	//strcpy(QryInstrument.InstrumentID,"IF1206");
	g_puserapi->ReqQryInstrument(&QryInstrument,g_nOrdLocalID++);
	return ;
}

void StartQryTradingCode()
{
	CUstpFtdcQryTradingCodeField QryTradingCode;
	if (g_puserapi==NULL)
	{
		printf("StartQryInvestorAccount  USERAPIδ����");
		return ;

	}
	strcpy(QryTradingCode.ExchangeID,"CFFEX");
	strcpy(QryTradingCode.BrokerID,g_BrokerID);
	strcpy(QryTradingCode.UserID,g_UserID);
	//strcpy(QryTradingCode.InvestorID,"1");
	g_puserapi->ReqQryTradingCode(&QryTradingCode,g_nOrdLocalID++);
	return ;
}

void StartQryInvestorPosition()
{
	CUstpFtdcQryInvestorPositionField QryInvestorPosition;
	memset(&QryInvestorPosition,0,sizeof(CUstpFtdcQryInvestorPositionField));
	if (g_puserapi==NULL)
	{
		printf("StartQryInvestorPosition  USERAPIδ����");
		return ;
	}
	strcpy(QryInvestorPosition.ExchangeID,"CFFEX");
	strcpy(QryInvestorPosition.BrokerID,g_BrokerID);
	printf("��Ͷ���߱�ţ�");
	scanf("%s",(QryInvestorPosition.InvestorID));
//	strcpy(QryInvestorPosition.InstrumentID,"");
	g_puserapi->ReqQryInvestorPosition(&QryInvestorPosition,g_nOrdLocalID++);
	return;
}

void StartUpdatePassword()
{
	CUstpFtdcUserPasswordUpdateField PasswordUpd;
	memset(&PasswordUpd,0,sizeof(CUstpFtdcUserPasswordUpdateField));
	if (g_puserapi==NULL)
	{
		printf("StartQryInvestorPosition  USERAPIδ����");
		return ;
	}
	strcpy(PasswordUpd.BrokerID,g_BrokerID);
	strcpy(PasswordUpd.UserID,g_UserID);
	printf("���������룺");
	scanf("%s",(PasswordUpd.OldPassword));
	printf("�����������룺");
	scanf("%s",(PasswordUpd.NewPassword));
	g_puserapi->ReqUserPasswordUpdate(&PasswordUpd,g_nOrdLocalID++);
	return;
}

void StartLogOut()
{
	CUstpFtdcReqUserLogoutField UserLogOut;
	memset(&UserLogOut,0,sizeof(CUstpFtdcReqUserLogoutField));
	strcpy(UserLogOut.BrokerID,g_BrokerID);
	strcpy(UserLogOut.UserID,g_UserID);
	
	g_puserapi->ReqUserLogout(&UserLogOut,g_nOrdLocalID++);
}

void StartQueryFee()
{
	printf("�ڲ�ѯ��������\n");
	CUstpFtdcQryInvestorFeeField InvestorFee;
	memset(&InvestorFee,0,sizeof(CUstpFtdcQryInvestorFeeField));
	strcpy(InvestorFee.BrokerID,g_BrokerID);
	strcpy(InvestorFee.UserID,g_UserID);
	g_puserapi->ReqQryInvestorFee(&InvestorFee,g_nOrdLocalID++);
}

void StartQueryMargin()
{
	printf("�ڲ�ѯ��֤����\n");
	CUstpFtdcQryInvestorMarginField InvestorMargin;
	memset(&InvestorMargin,0,sizeof(CUstpFtdcQryInvestorMarginField));
	strcpy(InvestorMargin.BrokerID,g_BrokerID);
	strcpy(InvestorMargin.UserID,g_UserID);
	g_puserapi->ReqQryInvestorMargin(&InvestorMargin,g_nOrdLocalID++);
}

void StartQueryComplianceParam()
{
	printf("�ڲ�ѯ�Ϲ����\n");
	CUstpFtdcQryComplianceParamField ComplianceParam;
	memset(&ComplianceParam,0,sizeof(CUstpFtdcQryComplianceParamField));
	strcpy(ComplianceParam.BrokerID,g_BrokerID);
	strcpy(ComplianceParam.UserID,g_UserID);
	printf("��Ͷ���߱�ţ�");
	scanf("%s",(ComplianceParam.InvestorID));
	g_puserapi->ReqQryComplianceParam(&ComplianceParam,g_nOrdLocalID++);
}

#ifdef WIN32
int WINAPI OrderFunc(LPVOID pParam)
#else
void * OrderFunc(void *pParam)
#endif
{
	while(1){
		ShowManu();
	 	printf("g_choose=[%d]\n",g_choose);
	 	switch(g_choose)
	 	{
	 		case 0:
	 			StartLogOut();
	 			exit(0);
	 			
	 		case 1:
	 			StartInputOrder ();
	 			
	 			break;
	 		case 2:
	 			StartOrderAction();
	 			break;
	 		case 3:
	 			StartQryOrder();
	 			break;
	 		case 4:
	 			StartQryTrade();
	 			break;
	 		case 5:
	 			StartQryInvestorAccount();
	 			break;
	 		case 6:
	 			StartQryTradingCode();
	 			break;
	 		case 7:
	 			StartQueryExchange();
	 			break;
	 		case 8:
	 			StartQryInstrument();
	 			break;
	 		case 9:
	 			StartQryUserInvestor();
	 			break;
	 		case 10:
	 			StartQryInvestorPosition();
	 			break;
	 		case 11:
	 			StartUpdatePassword();
	 			break;
			case 12:
				StartQueryFee();
				break;
			case 13:
				StartQueryMargin();
				break;
			case 14:
				StartQueryComplianceParam();
				break;
	 		default:
	 			printf("Input Error\n");
	 			break;	
	 		
	 		
	 		
	 	}
	 	Sleep(200);
	}
	return 0;
}
bool StartAutoOrder()
{	
	//int dwIDThread;
	unsigned long dwIDThread;
	THREAD_HANDLE hThread;	/**< �߳̾�� */
	bool ret = true;
#ifdef WIN32
	hThread = ::CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)OrderFunc,NULL,0,&dwIDThread);
	if(hThread==NULL)
	{
		ret = false;
	}
	SetThreadPriority(hThread,THREAD_PRIORITY_TIME_CRITICAL);
	ResumeThread(hThread);
#else
	ret = (::pthread_create(&hThread,NULL,&OrderFunc , NULL) == 0);
	
#endif
	return ret;
}

void StartTest()
{
	switch(g_nSwitch){
		case 1:
		case 2:
		case 3:
		case 4:
			StartAutoOrder();
			break;
		case 5:
			printf("StartInputOrder \n");
			StartInputOrder();
			break;
		case 6:
			printf("StartOrderAction \n");
			StartOrderAction();
			break;
		case 7:
			printf("StartQueryExchange \n");
			StartQueryExchange();
			break;
		case 8:
			printf("StartQryInvestorAccount \n");
			StartQryInvestorAccount();
			break;
		case 9:
			printf("StartQryUserInvestor \n");
			StartQryUserInvestor();
			break;
		case 10:
			printf("StartQryInstrument \n");
			StartQryInstrument();
			break;
		case 11:
			printf("StartQryTradingCode \n");
			StartQryTradingCode();
			break;
		default :
			printf("Input arg Error\n");
			break;
	}

	return ;
}