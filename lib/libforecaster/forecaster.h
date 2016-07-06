#ifndef FORECASTER_H_INCLUDED
#define FORECASTER_H_INCLUDED

#include "ChinaL1DiscreteFeature.h"
#include <map>
#include <vector>
#include <string>


typedef std::vector<ChinaL1DiscreteFeature *> FeatureVector;
typedef std::map<std::string, FeatureVector> FeatureMap;

class Forecaster
{
public:

    Forecaster(){}

    void update(const ChinaL1Msg& msg);

    double get_forecast();

    void reset();

protected:
    void addSelfFeature(std::string instr, ChinaL1DiscreteFeature* feature);
    void addOtherFeature(std::string instr, std::string ref_instrument, ChinaL1DiscreteFeature* feature);
    FeatureMap m_feaMap;
    std::string m_trading_instrument;


};


class ForecasterFactory
{
public:
    static Forecaster * createForecaster(std::string& fcName, std::string& date);

};


class HC_0_Forecaster:public Forecaster
{

public:
    HC_0_Forecaster(std::string& date);
};

#endif // FORECASTER_H_INCLUDED
