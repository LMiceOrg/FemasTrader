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
	double m_price_tick;
}ST_CUR_MD,*ST_CUR_MD_P;

typedef struct current_status{
	char m_ins_name[16];
	ST_POS m_pos;
	ST_ACC m_acc;
	ST_CUR_MD m_md;
}CUR_STATUS,*CUR_STATUS_P;

#endif
