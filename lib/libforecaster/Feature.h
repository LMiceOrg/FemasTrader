#ifndef FEATURE_H_INCLUDED
#define FEATURE_H_INCLUDED

#include <stdint.h>
class Feature
{
public:
    Feature(double weight);
    double get_signal() const { return m_weight * this->get_value();}
    virtual double get_value() const = 0;
    virtual void reset() = 0;
    virtual ~Feature() {}

private:
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
