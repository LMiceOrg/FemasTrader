#include "lmspi.h"
#include "fmspi.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>


#define SOCK_FILE "/var/run/lmiced.socket"

CLMSpi::CLMSpi(const char *name)
{
    m_name = name;
    create_uds_msg((void**)&sid);
    init_uds_client(SOCK_FILE, sid);
    logging("LMice %s running %d:%d", m_name.c_str(), getuid(), getpid());
    memset(&m_info, 0, sizeof(m_info));
    m_info.pid = getpid();
    m_info.tid = eal_gettid();
    m_info.type = LMICE_TRACE_TYPE;
    lmice_trace_info_t* pinfo = (lmice_trace_info_t*)sid->data;
    pinfo->loglevel=0;
    pinfo->pid = m_info.pid;
    pinfo->tid = m_info.tid;
    time(&pinfo->tm);
    get_system_time(&pinfo->systime);

}

CLMSpi::~CLMSpi()
{
    logging("LMice %s stopped.\n\n", m_name.c_str());
    finit_uds_msg(sid);
}

void CLMSpi::logging(const char* format, ...) {
    va_list argptr;

    time(&m_info.tm);
    get_system_time(&m_info.systime);
    memset(sid->data, 0, sizeof(sid->data));
    memcpy(sid->data, &m_info, sizeof(m_info) );
    sid->size = sizeof(m_info);

    va_start(argptr, format);
    sid->size += vsprintf(sid->data+sizeof(m_info), format, argptr);
    va_end(argptr);

    send_uds_msg(sid);
}

static int code_convert(const char *from_charset,const char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
        iconv_t cd;
        int rc;
        char **pin = &inbuf;
        char **pout = &outbuf;

        cd = iconv_open(to_charset,from_charset);
        if (cd==0)
                return -1;
        memset(outbuf,0,outlen);
        if (iconv(cd,pin,&inlen,pout,&outlen) == -1)
                return -1;
        iconv_close(cd);
        return 0;
}

std::string CLMSpi::gbktoutf8( char *pgbk)
{
    size_t inlen = strlen(pgbk);
    char outbuf[512] ={0};
    size_t outlen = 512;
    code_convert("gb2312","utf-8",pgbk,inlen,(char*)outbuf,outlen);

    std::string utf8;
    utf8.insert(utf8.begin(), outbuf, outbuf+strlen(outbuf));
    return utf8;

}

void CLMSpi::subscribe(const char *symbol)
{
    lmice_sub_t *psub = (lmice_sub_t *)sid->data;
    lmice_trace_info_t *pinfo = &psub->info;

    time(&pinfo->tm);
    get_system_time(&pinfo->systime);
    pinfo->type = EM_LMICE_SUB_TYPE;
    strncpy(psub->symbol, symbol, strlen(symbol)>sizeof(psub->symbol)?sizeof(psub->symbol):strlen(symbol));

    send_uds_msg(sid);
}

void CLMSpi::unsubscribe(const char *symbol)
{
    lmice_unsub_t *psub = (lmice_unsub_t *)sid->data;
    lmice_trace_info_t *pinfo = &psub->info;

    time(&pinfo->tm);
    get_system_time(&pinfo->systime);
    pinfo->type = EM_LMICE_UNSUB_TYPE;
    strncpy(psub->symbol, symbol, strlen(symbol)>sizeof(psub->symbol)?sizeof(psub->symbol):strlen(symbol));

    send_uds_msg(sid);
}

int CLMSpi::cancel(int requestId, int sysId)
{
    int req;
    CTraderSpi* spi = static_cast<CTraderSpi*>(this);
    CUstpFtdcOrderActionField ord;
    memset(&ord,0, sizeof(ord));

    requestId = spi->GetRequestID();
    ///交易所代码
    memcpy(ord.ExchangeID, EXCHANGE_ID, sizeof(EXCHANGE_ID)-1 );
    ///报单编号 优先级最高，填写系统返回单号，撤报单
    if(sysId) {
        sprintf(ord.OrderSysID, "%012d", sysId);
    }

    ///经纪公司编号
    memcpy(ord.BrokerID, spi->GetConf()->g_BrokerID, sizeof(ord.BrokerID) );
    ///投资者编号
    memcpy(ord.InvestorID, spi->GetConf()->g_InvestorID, sizeof(ord.InvestorID) );
    ///用户代码
    memcpy(ord.UserID, spi->GetConf()->g_UserID, sizeof(ord.UserID) );
    ///本次撤单操作的本地编号
    //TUstpFtdcUserOrderLocalIDType	UserOrderActionLocalID;
    sprintf(ord.UserOrderActionLocalID, "%012d",requestId);

    ///被撤订单的本地报单编号
//    TUstpFtdcUserOrderLocalIDType	UserOrderLocalID;
    sprintf(ord.UserOrderLocalID, "%012d", req);
    ///报单操作标志 ‘0’
//    TUstpFtdcActionFlagType	ActionFlag;
    ord.ActionFlag =USTP_FTDC_AF_Delete;
    /// 下面字段可空
    ///价格
    //TUstpFtdcPriceType	LimitPrice;
    ///数量变化
    //TUstpFtdcVolumeType	VolumeChange;
    ///本地业务标识
    //TUstpFtdcBusinessLocalIDType	BusinessLocalID;

    /// 发出撤单操作
    spi->GetTrader()->ReqOrderAction(&ord, req);

    return req;

}

int CLMSpi::order(const char *symbol, int dir, double price, int num)
{
    CTraderSpi* spi = static_cast<CTraderSpi*>(this);
    CUstpFtdcInputOrderField ord;
    memset(&ord, 0, sizeof(ord));

    ///经纪公司编号
    TUstpFtdcBrokerIDType	BrokerID;
    ///交易所代码
    TUstpFtdcExchangeIDType	ExchangeID;
    ///系统报单编号 。置空
    TUstpFtdcOrderSysIDType	OrderSysID;
    ///投资者编号
    TUstpFtdcInvestorIDType	InvestorID;
    ///用户代码
    TUstpFtdcUserIDType	UserID;
    ///合约代码
    TUstpFtdcInstrumentIDType	InstrumentID;
    ///用户本地报单号 。用户自己维护，用来跟踪报单状态 integer,  递增，%012d
    TUstpFtdcUserOrderLocalIDType	UserOrderLocalID;
    ///报单类型
    TUstpFtdcOrderPriceTypeType	OrderPriceType;
    ///买卖方向
    TUstpFtdcDirectionType	Direction;
    ///开平标志
    TUstpFtdcOffsetFlagType	OffsetFlag;
    ///投机套保标志
    TUstpFtdcHedgeFlagType	HedgeFlag;
    ///价格
    TUstpFtdcPriceType	LimitPrice;
    ///数量
    TUstpFtdcVolumeType	Volume;
    ///有效期类型
    TUstpFtdcTimeConditionType	TimeCondition;
    ///GTD日期 yyyymmdd
    TUstpFtdcDateType	GTDDate;
    ///成交量类型
    TUstpFtdcVolumeConditionType	VolumeCondition;
    ///最小成交量
    TUstpFtdcVolumeType	MinVolume;
    ///止损价
    TUstpFtdcPriceType	StopPrice;
    ///强平原因
    TUstpFtdcForceCloseReasonType	ForceCloseReason;
    ///自动挂起标志
    TUstpFtdcBoolType	IsAutoSuspend;
    ///业务单元 。没有用
    TUstpFtdcBusinessUnitType	BusinessUnit;
    ///用户自定义域 。服务器原样返回
    TUstpFtdcCustomType	UserCustom;
    ///本地业务标识 。没有用
    TUstpFtdcBusinessLocalIDType	BusinessLocalID;
    ///业务发生日期 。交易日
    TUstpFtdcDateType	ActionDay;

    return 0;

}


