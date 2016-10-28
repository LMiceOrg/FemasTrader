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



#pragma pack(1)//这个不能少，字节对齐方式
typedef struct MarketField
{
	int	   packetLen;//报文长度 4	0
	unsigned char versionNo;//版本序号	//	1	4
	int	   updateTime;//修改时间	//	4	5
	char   exchangeID[3];//交易所	//	3	9
	char   instrumentID[30];//合约代码	//	30	12
	bool   stopFlag;//停牌标识	//	1	42
	float  preSettlementPrice;//昨结算价	//	4	43
	float  settlementPrice;//今结算价	//	4	47
	float  averageMatchPrice;//成交均价	//	4	51
	float  yesterdayClosePrice;//昨收盘	//	4	55
	float  todayClosePrice;//今收盘	//	4	59
	float  todayOpenPrice;//今开盘	//	4	63
	int	   yesterdayPositionAmount;//昨持仓量	//	4	67
	int	   positionAmount;//持仓量	//	4	71
	float  latetestPrice;//最新价	//	4	75
	int    macthAmount;//成交数量	//	4	79
	double macthTotalMoney;//成交金额	//	8	83
	float  highestPrice;//最高报价	//	4	91
	float  lowestPrice;//最低报价	//	4	95
	float  upperPrice;//涨停板价	//	4	99
	float  lowerPrice;//跌停板价	//	4	103
	float  yesterdayImagineRealDegree;//昨虚实度	//	4	107
	float  imagineRealDegree;//今虚实度	//	4	111
	float  buyPrice1;//申买价1	//	4	115
	float  sellPrice1;//申卖价1	//	4	119
	int	   buyAmount1;//申买量1	//	4	123
	int	   sellAmount1;//申卖量1	//	4	127
	float  buyPrice2;//申买价2	//	4	131
	float  sellPrice2;//申卖价2	//	4	135
	int    buyAmount2;//申买量2	//	4	139
	int    sellAmount2;//申卖量2	//	4	143
	float  buyPrice3;//申买价3	//	4	147
	float  sellPrice3;//申卖价3	//	4	151
	int	   buyAmount3;//申买量3	//	4	155
	int	   sellAmount3;//申卖量3	//	4	159
	float  buyPrice4;//申买价4	//	4	163
	float  sellPrice4;//申卖价4	//	4	167
	int	   buyAmount4;//申买量4	//	4	171
	int    sellAmount4;//申卖量4	//	4	175
	float  buyPrice5;//申买价5	//	4	179
	float  sellPrice5;//申卖价5	//	4	183
	int    buyAmount5;//申买量5	//	4	187
	int	   sellAmount5;//申卖量5	//	4	191
	//无效	无效	//	61	195-255
}XQN_MD,*P_XQN_MD;
#pragma pack()

#endif
