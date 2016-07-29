#ifndef CHINAL1DISCRETEFEATURE_H_INCLUDED
#define CHINAL1DISCRETEFEATURE_H_INCLUDED

#include "Feature.h"
#include <string>
#include <stdint.h>

class ChinaL1Msg {
public:
	ChinaL1Msg();
	const std::string& get_inst() const {
		return m_inst;
	}
	int64_t get_time() const {
		return m_time_micro;
	}
	double get_bid() const {
		return m_bid;
	}
	double get_offer() const {
		return m_offer;
	}
	double get_bid_quantity() const {
		return m_bid_quantity;
	}
	double get_offer_quantity() const {
		return m_offer_quantity;
	}
	double get_volume() const {
		return m_volume;
	}
	double get_notional() const {
		return m_notional;
	}
	double get_limit_up() const {
		return m_limit_up;
	}
	double get_limit_down() const {
		return m_limit_down;
	}

	void set_inst(const char * inst);
	void set_time(int64_t time);
	void set_bid(double bid);
	void set_offer(double offer);
	void set_bid_quantity(int32_t bid_quantity);
	void set_offer_quantity(int32_t offer_quantity);
	void set_volume(int32_t volume);
	void set_notinal(double notional);
	void set_limit_up(double limit_up);
	void set_limit_down(double limit_down);

private:
	std::string m_inst;
	int64_t m_time_micro; // time in epoch micro seconds
	double m_bid;
	double m_offer;
	int32_t m_bid_quantity;
	int32_t m_offer_quantity;
	int32_t m_volume;
	double m_notional;
	double m_limit_up;
	double m_limit_down;
};

class IChinaL1DiscreteListner {

public:
	virtual void handleMsg(const ChinaL1Msg& msg) = 0;
	virtual ~IChinaL1DiscreteListner() {
	}
};

class ChinaL1DiscreteFeature: public Feature, public IChinaL1DiscreteListner {
public:
	ChinaL1DiscreteFeature(double weight);
	virtual ~ChinaL1DiscreteFeature() {
	}

};

class ChinaL1DiscreteSelfFeature: public ChinaL1DiscreteFeature {
public:
	ChinaL1DiscreteSelfFeature(double weight, std::string& trading_instrument);
	void handleMsg(const ChinaL1Msg& msg);
	double get_value() const {
		return m_signal;
	}
	virtual void reset() {
		m_signal = 0;
	}
	virtual ~ChinaL1DiscreteSelfFeature() {
	}

protected:
	virtual void handleSelfMsg(const ChinaL1Msg& msg) = 0;
	double m_signal;

private:
	const std::string m_trading_instrument;
};

class ChinaL1DiscreteOtherFeature: public ChinaL1DiscreteFeature {
public:
	ChinaL1DiscreteOtherFeature(double weight, std::string& trading_instrument,
			std::string& reference_instrument, const int64_t cutoff_time);
	void handleMsg(const ChinaL1Msg& msg);
	double get_value() const {
		return m_signal;
	}
	virtual void reset() {
		m_signal = 0;
	}
	virtual ~ChinaL1DiscreteOtherFeature() {
	}

protected:
	virtual void handleSelfMsg(const ChinaL1Msg& msg) = 0;
	virtual void handleOtherMsg(const ChinaL1Msg& msg) = 0;
	double m_signal;

private:
	const std::string m_trading_instrument;
	const std::string m_reference_instrument;
	const int64_t m_cutoff_time;
	double m_filter;

};

class ChinaL1DiscreteSelfTradeImbalanceFeature: public ChinaL1DiscreteSelfFeature {
public:
	ChinaL1DiscreteSelfTradeImbalanceFeature(double weight,
			std::string& trading_instrument, double contract_size);
	void reset();

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);

private:
	double const m_contract_size;
	double m_prev_time;
	double m_prev_bid;
	double m_prev_offer;
	double m_prev_volume;
	double m_prev_notional;

};

class ChinaL1DiscreteSelfBookImbalanceFeature: public ChinaL1DiscreteSelfFeature {
public:
	ChinaL1DiscreteSelfBookImbalanceFeature(double weight,
			std::string& trading_instrument, double tick_size);

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);

private:
	double const m_tick_size;

};

class ChinaL1DiscreteSelfDecayingReturnFeature: public ChinaL1DiscreteSelfFeature {
public:
	ChinaL1DiscreteSelfDecayingReturnFeature(double weight,
			std::string& trading_instrument, double decay_mics);
	void reset();

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);

private:
	EMA m_ema;

};

class ChinaL1DiscreteOtherTranslationFeature: public ChinaL1DiscreteOtherFeature {
public:
	ChinaL1DiscreteOtherTranslationFeature(double weight,
			std::string& trading_instrument, std::string& reference_instrument,
			int64_t cutoff_time, double ratio_mics, bool use_diff,
			bool corr_negative);
	virtual void reset();
	virtual ~ChinaL1DiscreteOtherTranslationFeature() {
	}

protected:
	double calculatedRatio(double mid, double other_mid) const {
		return m_corr_negative ?
				(m_use_diff ? (mid + other_mid) : (mid * other_mid)) :
				(m_use_diff ? (other_mid - mid) : (other_mid / mid));
	}
	double translatedPrice(double other_price) const {
		return m_corr_negative ?
				(m_use_diff ?
						(m_ema.get_value() - other_price) :
						(m_ema.get_value() / other_price)) :
				(m_use_diff ?
						(other_price - m_ema.get_value()) :
						(other_price / m_ema.get_value()));

	}

	double translatedDelta(double other_delta, double other_mid) const {
		return m_use_diff ?
				other_delta :
				(m_corr_negative ?
						other_delta * m_ema.get_value() / other_mid
								/ other_mid :
						other_delta * m_ema.get_value());

	}

	double translatedBid(double other_bid, double other_offer) const {

		return m_corr_negative ?
				translatedPrice(other_offer) : translatedPrice(other_bid);

	}

	double translatedOffer(double other_bid, double other_offer) const {

		return m_corr_negative ?
				translatedPrice(other_bid) : translatedPrice(other_offer);
	}

	double signedSignal(double signal) const {
		return m_corr_negative ? (-signal) : signal;
	}

	EMA m_ema;
private:
	bool const m_use_diff;
	bool const m_corr_negative;
};

class ChinaL1DiscreteOtherTradeImbalanceFeature: public ChinaL1DiscreteOtherTranslationFeature {
public:
	ChinaL1DiscreteOtherTradeImbalanceFeature(double weight,
			std::string& trading_instrument, std::string& reference_instrument,
			int64_t cutoff_time, double ratio_mics, bool use_diff,
			bool corr_negative, double contract_size);
	void reset();

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);
	void handleOtherMsg(const ChinaL1Msg& msg);

private:
	double const m_contract_size;
	double m_cur_other_bid;
	double m_cur_other_offer;
	double m_cur_other_volume;
	double m_cur_other_notional;
	double m_prev_other_bid;
	double m_prev_other_offer;
	double m_prev_other_volume;
	double m_prev_other_notional;
	double m_prev_time;
	double m_prev_bid;
	double m_prev_offer;

};

class ChinaL1DiscreteOtherBookImbalanceFeature: public ChinaL1DiscreteOtherTranslationFeature {
public:
	ChinaL1DiscreteOtherBookImbalanceFeature(double weight,
			std::string& trading_instrument, std::string& reference_instrument,
			int64_t cutoff_time, double ratio_mics, bool use_diff,
			bool corr_negative, double tick_size);
	void reset();

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);
	void handleOtherMsg(const ChinaL1Msg& msg);

private:
	double const m_tick_size;

	double m_cur_other_bid;
	double m_cur_other_offer;
	double m_cur_other_bid_quantity;
	double m_cur_other_offer_quantity;
	double m_prev_bid;
	double m_prev_offer;

};

class ChinaL1DiscreteOtherBestSinceFeature: public ChinaL1DiscreteOtherTranslationFeature {
public:
	ChinaL1DiscreteOtherBestSinceFeature(double weight,
			std::string& trading_instrument, std::string& reference_instrument,
			int64_t cutoff_time, double ratio_mics, bool use_diff,
			bool corr_negative);
	void reset();

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);
	void handleOtherMsg(const ChinaL1Msg& msg);

private:

	double m_cur_other_bid;
	double m_cur_other_offer;
	double m_prev_bid;
	double m_prev_offer;

};

class ChinaL1DiscreteOtherDecayingReturnFeature: public ChinaL1DiscreteOtherFeature {
public:
	ChinaL1DiscreteOtherDecayingReturnFeature(double weight,
			std::string& trading_instrument, std::string& reference_instrument,
			int64_t cutoff_time, double decay_mics);
	void reset();

protected:
	void handleSelfMsg(const ChinaL1Msg& msg);
	void handleOtherMsg(const ChinaL1Msg& msg);

private:
	EMA m_ema;
	double m_cur_other_bid;
	double m_cur_other_offer;

};
#endif // CHINAL1DISCRETEFEATURE_H_INCLUDED
