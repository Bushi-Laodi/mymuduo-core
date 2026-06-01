#pragma once
#include "mymuduo/Channel.h"
#include "mymuduo/Timer.h"
#include <set>
#include <vector>

namespace mymuduo {

class EventLoop;

class TimerQueue {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

private:
    using Entry = std::pair<Timestamp, Timer*>;//{ 定时器到期时间, Timer 指针 }
    using TimerList = std::set<Entry>;//默认按照第一个成员排序，需要写一下第一个的 < 函数

    void addTimerInLoop(Timer* timer);
    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;
};

}