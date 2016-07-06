#ifndef _FIELD_HELPER_H_
#define _FIELD_HELPER_H_

#include <stdio.h>
#include <stdarg.h>

//#include "USTPFtdcMduserApi.h"

#define DoubleFormat(buf, maxlen, val) \
    ((val == DBL_MAX) ? StringFormat(buf, (maxlen), ",") : StringFormat(buf, (maxlen), "%.5f,", val))

static inline int StringFormat(char *buf, int maxlen, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = vsnprintf(buf, maxlen, fmt, va);
    va_end(va);
    return ret > 0 ? ret : 0;
}


template<typename type>
static inline type min_2(type a, type b){
    return ((a) < (b) ? (a) : (b));
}
//// 增加对字段长度的限制，在处理多播行情包的时候特别有用
////scylla
////template<typename MarketFieldDetail>
////scylla
//static inline int MarketFieldToBuffer(char *buf, int maxlen, CUstpFtdcDepthMarketDataField* field) {
//    int len = 0;
//    int field_max_length = 23;
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%s,", field->TradingDay);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%s,", field->SettlementGroupID);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->SettlementID);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->PreSettlementPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->PreClosePrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->PreOpenInterest);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->PreDelta);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->OpenPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->HighestPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->LowestPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->ClosePrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->UpperLimitPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->LowerLimitPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->SettlementPrice);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->CurrDelta);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->LastPrice);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->Volume);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->Turnover);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->OpenInterest);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->BidPrice1);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->BidVolume1);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->AskPrice1);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->AskVolume1);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->BidPrice2);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->BidVolume2);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->AskPrice2);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->AskVolume2);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->BidPrice3);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->BidVolume3);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->AskPrice3);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->AskVolume3);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->BidPrice4);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->BidVolume4);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->AskPrice4);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->AskVolume4);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->BidPrice5);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->BidVolume5);
//    len += DoubleFormat(buf + len, min_2(maxlen - len, field_max_length), field->AskPrice5);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%d,", field->AskVolume5);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%s,", field->InstrumentID);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%s,", field->UpdateTime);
//    len += StringFormat(buf + len, min_2(maxlen - len, field_max_length), "%ld", field->UpdateMillisec);
//    //*(buf + len) = '\n';
//    return len;
//}




#endif
