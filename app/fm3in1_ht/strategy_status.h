#ifndef _STRATEGY_STATUS_H
#define _STRATEGY_STATUS_H

typedef struct status_pos{
    int m_yd_buy_pos;
    int m_yd_sell_pos;
    int m_buy_pos;
    int m_sell_pos;
}ST_POS,*ST_POS_P;

typedef struct status_account{
	int m_count_id;
	double m_left_cash;
	double m_total_val;
	double m_valid_val;
}ST_ACC,*ST_ACC_P;

typedef struct status_current_md{
	int m_multiple;
	double fee_rate;
	double m_up_price;
	double m_down_price;
	double m_last_price;
}ST_CUR_MD,*ST_CUR_MD_P;

typedef struct current_status{
	char m_ins_name[16];
	ST_POS m_pos;
	ST_ACC m_acc;
	ST_CUR_MD m_md;
}CUR_STATUS,*CUR_STATUS_P;



#pragma pack(1)//��������٣��ֽڶ��뷽ʽ
typedef struct MarketField
{
	int	   packetLen;//���ĳ��� 4	0
	unsigned char versionNo;//�汾���	//	1	4
	int	   updateTime;//�޸�ʱ��	//	4	5
	char   exchangeID[3];//������	//	3	9
	char   instrumentID[30];//��Լ����	//	30	12
	bool   stopFlag;//ͣ�Ʊ�ʶ	//	1	42
	float  preSettlementPrice;//������	//	4	43
	float  settlementPrice;//������	//	4	47
	float  averageMatchPrice;//�ɽ�����	//	4	51
	float  yesterdayClosePrice;//������	//	4	55
	float  todayClosePrice;//������	//	4	59
	float  todayOpenPrice;//����	//	4	63
	int	   yesterdayPositionAmount;//��ֲ���	//	4	67
	int	   positionAmount;//�ֲ���	//	4	71
	float  latetestPrice;//���¼�	//	4	75
	int    macthAmount;//�ɽ�����	//	4	79
	double macthTotalMoney;//�ɽ����	//	8	83
	float  highestPrice;//��߱���	//	4	91
	float  lowestPrice;//��ͱ���	//	4	95
	float  upperPrice;//��ͣ���	//	4	99
	float  lowerPrice;//��ͣ���	//	4	103
	float  yesterdayImagineRealDegree;//����ʵ��	//	4	107
	float  imagineRealDegree;//����ʵ��	//	4	111
	float  buyPrice1;//�����1	//	4	115
	float  sellPrice1;//������1	//	4	119
	int	   buyAmount1;//������1	//	4	123
	int	   sellAmount1;//������1	//	4	127
	float  buyPrice2;//�����2	//	4	131
	float  sellPrice2;//������2	//	4	135
	int    buyAmount2;//������2	//	4	139
	int    sellAmount2;//������2	//	4	143
	float  buyPrice3;//�����3	//	4	147
	float  sellPrice3;//������3	//	4	151
	int	   buyAmount3;//������3	//	4	155
	int	   sellAmount3;//������3	//	4	159
	float  buyPrice4;//�����4	//	4	163
	float  sellPrice4;//������4	//	4	167
	int	   buyAmount4;//������4	//	4	171
	int    sellAmount4;//������4	//	4	175
	float  buyPrice5;//�����5	//	4	179
	float  sellPrice5;//������5	//	4	183
	int    buyAmount5;//������5	//	4	187
	int	   sellAmount5;//������5	//	4	191
	//��Ч	��Ч	//	61	195-255
}XQN_MD,*P_XQN_MD;
#pragma pack()

#endif
