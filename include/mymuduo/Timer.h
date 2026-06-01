#pragma once
#include "mymuduo/Callbacks.h"
#include "mymuduo/Timestamp.h"

namespace mymuduo {

class Timer {
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)), expiration_(when), interval_(interval), repeat_(interval > 0.0) {}
    void run() const { callback_(); }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    void restart(Timestamp now) { if (repeat_) expiration_ = addTime(now, interval_); }

    static Timestamp addTime(Timestamp now, double add){
        Timestamp result(static_cast<int64_t>(add * 1000 * 1000) + now.microSecondsSinceEpoch());
        return result;
    }
    
private:
    

    TimerCallback callback_;
    Timestamp expiration_;
    double interval_;
    bool repeat_;
};

class TimerId {
public:
    explicit TimerId(Timer* timer = nullptr) : timer_(timer) {}
private:
    Timer* timer_;
};



}