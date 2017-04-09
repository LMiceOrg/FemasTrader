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
	virtual ~Forecaster();

    void update(const ChinaL1Msg& msg);

    double get_forecast();

    void get_all_signal( double **res, int *size );

    void reset();

    std::string get_trading_instrument() const{ return m_trading_instrument; }
    std::vector<std::string> get_subscriptions() const;
	std::vector<std::string> get_subsignal_name() const;

protected:
    void addSelfFeature(std::string instr, ChinaL1DiscreteFeature* feature);
    void addOtherFeature(std::string instr, std::string ref_instrument, ChinaL1DiscreteFeature* feature);
    FeatureMap m_feaMap;
    std::string m_trading_instrument;

    std::vector<std::string> m_subscriptions;
	std::vector<std::string> m_subsignal_name;

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

class RU2_0_Forecaster:public Forecaster
{

public:
    RU2_0_Forecaster(struct tm& date);
};

class RU2_0_New_Forecaster:public Forecaster
{
public:
	RU2_0_New_Forecaster(struct tm& date);		
};


class RB2_New_Forecaster:public Forecaster
{
public:
	RB2_New_Forecaster(struct tm& date);		
};


class NI2_0_Forecaster:public Forecaster
{

public:
    NI2_0_Forecaster(struct tm& date);
};

class NI2_New_Forecaster:public Forecaster
{

public:
    NI2_New_Forecaster(struct tm& date);
};

class ZN2_0_Forecaster:public Forecaster
{

public:
    ZN2_0_Forecaster(struct tm& date);
};

class ZN2_1_Forecaster:public Forecaster
{

public:
    ZN2_1_Forecaster(struct tm& date);
};

class ZN2_2_Forecaster:public Forecaster
{

public:
    ZN2_2_Forecaster(struct tm& date);
};

class AL2_0_Forecaster:public Forecaster
{

public:
    AL2_0_Forecaster(struct tm& date);
};


class CU2_0_Forecaster:public Forecaster
{

public:
    CU2_0_Forecaster(struct tm& date);
};

class JM1_0_Forecaster:public Forecaster
{
public:
	JM1_0_Forecaster(struct tm& date);
};

class P1_0_Forecaster:public Forecaster
{
public:
	P1_0_Forecaster(struct tm& date);
};

class L1_0_Forecaster:public Forecaster
{
public:
	L1_0_Forecaster(struct tm& date);
};

class RB_0_Forecaster:public Forecaster
{

public:
    RB_0_Forecaster(struct tm& date);
};

class RB_0_test_Forecaster:public Forecaster
{

public:
    RB_0_test_Forecaster(struct tm& date);
};


class RB2_0_Forecaster:public Forecaster
{

public:
    RB2_0_Forecaster(struct tm& date);
};

#if 1

class NI1_0_Forecaster:public Forecaster
{
public:
	NI1_0_Forecaster(struct tm& date);
};

class ZN1_0_Forecaster:public Forecaster
{
public:
	ZN1_0_Forecaster(struct tm& date);
};

class PB1_0_Forecaster:public Forecaster
{
public:
	PB1_0_Forecaster(struct tm& date);
};

class CU1_0_Forecaster:public Forecaster
{
public:
	CU1_0_Forecaster(struct tm& date);
};

class AL1_0_Forecaster:public Forecaster
{
public:
	AL1_0_Forecaster(struct tm& date);
};

class SN1_0_Forecaster:public Forecaster
{
public:
	SN1_0_Forecaster(struct tm& date);
};
#endif

#endif // FORECASTER_H_INCLUDED
