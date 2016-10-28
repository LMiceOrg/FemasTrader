#ifndef GUAVAPROTO_H
#define GUAVAPROTO_H

#define MAX_IP_LEN				32

#define QUOTE_FLAG_SUMMARY		4


#pragma pack(push, 1)

struct guava_udp_head
{
    unsigned int	m_sequence;				/* 会话编号 */
    char			m_exchange_id;			/* 市场  0 表示中金  1表示上期 */
    char			m_channel_id;			/* 通道编号 */
    char			m_symbol_type_flag;		/* 合约标志 */
    int				m_symbol_code;			/* 合约编号 */
    char			m_symbol[31];			/* 合约 */
    char			m_update_time[9];		/* 最后更新时间(秒) */
    int				m_millisecond;			/* 最后更新时间(毫秒) */
    char			m_quote_flag;			/* 行情标志
                                            0 无time sale, 无lev1,
                                            1 有time sale 无lev1,
                                            2 无time sale, 有lev1,
                                            3 有time sale, 有lev1,
                                            4 summary信息
                                             */
};

struct guava_udp_normal
{
    double			m_last_px;				/* 最新价 */
    int				m_last_share;			/* 最新成交量 */
    double			m_total_value;			/* 成交金额 */
    double			m_total_pos;			/* 持仓量 */
    double			m_bid_px;				/* 最新买价 */
    int				m_bid_share;			/* 最新买量 */
    double			m_ask_px;				/* 最新卖价 */
    int				m_ask_share;			/* 最新卖量 */
};

struct guava_udp_summary
{
    double			m_open;					/* 今开盘 */
    double			m_high;					/* 最高价 */
    double			m_low;					/* 最低价 */
    double			m_today_close;			/* 今收盘 */
    double			m_high_limit;			/* 涨停价 */
    double			m_low_limit;			/* 跌停价 */
    double			m_today_settle;			/* 今结算价 */
    double			m_curr_delta;			/* 今虚实度 */
};

struct multicast_info
{
    char	m_remote_ip[MAX_IP_LEN];		/*  组播行情远端地址 */
    int		m_remote_port;					/*  组播行情远端端口 */
    char	m_local_ip[MAX_IP_LEN];			/*  组播本机地址 */
    int		m_local_port;					/*  组播本机端口 */
};


#pragma pack(pop)

typedef struct InstrumentTime{
    char InstrumentID[31];
    char   UpdateTime[9];
    unsigned int    UpdateMillisec;
} InstrumentTimeT, *pInstrumentTimeT;

typedef struct HighLow{
    double UpperLimitPrice;
    double LowerLimitPrice;
}HighLowT;

typedef struct PriceVolume{
    double LastPrice;
    int Volume;
    double Turnover;
    double OpenInterest;
}PriceVolumeT;

typedef struct AskBid{
    double BidPrice1;
    int BidVolume1;
    double AskPrice1;
    int AskVolume1;
}AskBidT;

typedef struct IncQuotaData{
    InstrumentTimeT InstTime;
    HighLowT HighLowInst;
    PriceVolumeT PriVol;
    AskBidT AskBidInst;
}IncQuotaDataT, *pIncQuotaDataT;

typedef struct SHFEQuotDataTag
{
	char            InstrumentID[31]; // ��Լ����
	char            fill1;            // 1�ֽ����
	char            UpdateTime[8];    // ����ʱ��
	char            ActionDay[8];     // �����������
	int             UpdateMillisec;   // ���º���
	int             Volume;           // �ɽ���
	double          LastPrice;        // ���¼�
	double          Turnover;         // �ɽ����
	double          OpenInterest;     // �ֲ���
	double          BidPrice1;        // ��һ��
	double          AskPrice1;        // ��һ��
	int             BidVolume1;       // ��һ��
	int             AskVolume1;       // ��һ��
} SHFEQuotDataT;

#endif /** GUAVAPROTO_H */
