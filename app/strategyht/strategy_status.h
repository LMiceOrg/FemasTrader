#ifndef _STRATEGY_STATUS_H
#define _STRATEGY_STATUS_H

typedef struct status_pos{
	double m_buy_count;
	double m_sell_count;
}ST_POS,*ST_POS_P;

typedef struct status_account{
	int m_count_id;
	double m_valid_money;
}ST_ACC,*ST_ACC_P;

/*
typedef struct status_pl{
	double m_pl;
}ST_PL,*ST_PL_P;
*/

typedef struct status_current_md{
	double m_last_price;
}ST_CUR_MD,*ST_CUR_MD_P;

typedef struct current_status{
	char m_ins_name[16];
	ST_POS m_pos;
	ST_ACC m_acc;
//	ST_PL m_pl;
	ST_CUR_MD m_md;
}CUR_STATUS,*CUR_STATUS_P;

#endif
