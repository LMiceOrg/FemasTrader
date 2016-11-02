#ifndef FORECASTER_H_INCLUDED
#define FORECASTER_H_INCLUDED

#include "ChinaL1DiscreteFeature.h"
#include "RollScheme.h"
#include <map>
#include <vector>
#include <string>
#include <time.h>


typedef std::vector<ChinaL1DiscreteFeature *> FeatureVector;
typedef std::map<std::string, FeatureVector> FeatureMap;

class Forecaster
{
public:

    Forecaster(){}

    void update(const ChinaL1Msg& msg);

    double get_forecast();

    void get_all_signal( double array[] );

    void reset();

    std::string get_trading_instrument() const{ return m_trading_instrument;}
    std::vector<std::string> get_subscriptions() const;

protected:
    void addSelfFeature(std::string instr, ChinaL1DiscreteFeature* feature);
    void addOtherFeature(std::string instr, std::string ref_instrument, ChinaL1DiscreteFeature* feature);
    FeatureMap m_feaMap;
    std::string m_trading_instrument;

    std::vector<std::string> m_subscriptions;

};


class ForecasterFactory
{
public:
    static Forecaster * createForecaster(std::string& fcName, struct tm& date);

};


class HC_0_Forecaster:public Forecaster
{

public:
    HC_0_Forecaster(struct tm& date);
};

class HC_1_Forecaster:public Forecaster
{

public:
    HC_1_Forecaster(struct tm& date);
};

class HC2_1_Forecaster:public Forecaster
{

public:
    HC2_1_Forecaster(struct tm& date);
};

class RU_0_Forecaster:public Forecaster
{

public:
    RU_0_Forecaster(struct tm& date);
};


class RB_0_Forecaster:public Forecaster
{

public:
    RB_0_Forecaster(struct tm& date);
};

class RB2_0_Forecaster:public Forecaster
{

public:
    RB2_0_Forecaster(struct tm& date);
};
#endif // FORECASTER_H_INCLUDED
