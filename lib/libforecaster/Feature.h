#ifndef FEATURE_H_INCLUDED
#define FEATURE_H_INCLUDED

#include <stdint.h>
#include <string>

class Feature
{
public:
    Feature(const std::string feautre_name, double weight);
	Feature(double weight);
    double get_signal() const { return m_weight * this->get_value();}
	const std::string& get_feature_name(){ return m_feature_name; }
    virtual double get_value() const = 0;
    virtual void reset() = 0;
    virtual ~Feature() {}

private:
	std::string m_feature_name;
    double const m_weight;
};

class EMA
{
public:
    EMA(double decay);
    void update(double value, double event);
    void reset();
    double get_value()const { return m_value;}

private:
    double const m_decay;
    bool m_initialized;
    double m_value;
    double m_last_event;


};

#endif // FEATURE_H_INCLUDED
