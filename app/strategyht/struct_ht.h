#ifndef _STRUCT_HT_H
#define _STURCT_HT_H

#include "publicfuncs.h"

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

#endif
