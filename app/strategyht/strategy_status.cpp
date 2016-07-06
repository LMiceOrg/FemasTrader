#include "strategy_status.h"

int strategy_status::set_pos( struct CUstpFtdcRspInvestorPositionField *serv_pos )
{
	if( NULL == serv_pos )
	{
		return -1;
	}

	if( serv_pos->Direction == USTP_FTDC_D_Buy )	
		m_ptr_pos->cur_pos = serv_pos->Position;
	else
		m_ptr_pos->cur_pos = -serv_pos->Position;
	memcpy(&m_ptr_pos->serv_pos, serv_pos, sizeof(CUstpFtdcRspInvestorPositionField));

	return m_ptr_pos->cur_pos;
}


double strategy_status::set_pl( PTRPL *serv_pl )
{
	if( NULL == serv_pl )
	{
		return -1;
	}

	m_ptr_pl->cur_pl = serv_pl->last_postion * m_ptr_md->cur_price * m_factor + serv_pl->last_pro_PL - serv_pl->total_fee;
	return m_ptr_pl->cur_pl;
}

int strategy_status::set_ac( struct CUstpFtdcRspInvestorAccountField *serv_ac )
{
	if( NULL == serv_ac )
	{
		return -1;
	}
	if( 0 == m_ptr_ac->open_fund)
	{
		m_ptr_ac->open_fund = serv_ac->Available;
	}
	else
	{
		m_ptr_ac->valied_fund = serv_ac->Available;
	}
	memcpy(m_ptr_ac->serv_ac, serv_ac, sizeof(struct CUstpFtdcRspInvestorAccountField));
}

double strategy_status::set_md( pIncQuotaDataT *serv_md )
{
	if( NULL == serv_md )
	{
		return -1;
	}

	m_ptr_md->cur_price = serv_md->PriVol.LastPrice;
	return m_ptr_md->cur_price;
}

