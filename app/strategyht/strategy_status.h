#ifndef _STRUCT_HT_H
#define _STRUCT_HT_H

#include "struct_ht.h"

typedef struct st_pos{
	int cur_pos;
	struct CUstpFtdcRspInvestorPositionField serv_pos;
}STPOS,*PSTPOS;

typedef struct st_pl{
	double cur_pl;
	TraderPL serv_pl;
}STPL,*PSTPL;

typedef struct st_account{
	double open_fund;
	double valied_fund;
	struct CUstpFtdcRspInvestorAccountField serv_ac;
}STAC,*PSTAC;

typedef struct st_md{
	double cur_price; 
}STMD,*PSTMD;

class strategy_status
{

public:
	strategy_status( const char * name="rb1610", double ins_factor )
	{
		m_ptr_pos = new STPOS;
		m_ptr_pl = new STPL;
		m_ptr_ac = new STAC;
		m_ptr_md = new STMD;

		memset( m_instrument, 0, 16 );
		strcpy( m_instrument, name );

		memset( m_ptr_pos, 0, sizeof(STPOS) );
		memset( m_ptr_pl, 0, sizeof(STPL) );
		memset( m_ptr_ac, 0, sizeof(STAC) );
		memset( m_ptr_md, 0, sizeof(STMD) );

		m_factor = ins_factor;
	}
	
	virtual ~strategy_status()
	{
		delete m_ptr_pos;
		delete m_ptr_ac;
		delete m_ptr_pl;
		delete m_ptr_md;
	}

	int get_pos(){ return m_ptr_pos->cur_pos; }
	double get_pl(){ return m_ptr_pl->cur_pl; }
	double get_valid_ac(){ return m_ptr_ac->valied_fund; }
	double get_open_ac(){ return m_ptr_ac->open_fund; }
	double get_md(){ return m_ptr_md->cur_price;}
	
	int set_pos( struct CUstpFtdcRspInvestorPositionField *serv_pos );
	double set_pl( PTRPL *serv_pl );
	double set_ac( struct CUstpFtdcRspInvestorAccountField *serv_ac );
	double set_md( pIncQuotaDataT serv_md );
	
private:
	char m_instrument[16];
	PSTPOS m_ptr_pos;	
	PSTPL  m_ptr_pl;
	PSTAC  m_ptr_ac;
	PSTMD  m_ptr_md;
	double m_factor;
};

#endif
