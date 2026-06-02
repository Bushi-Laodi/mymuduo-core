#pragma once 
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include "mymuduo/Timestamp.h"

namespace mymuduo{

class Channel;
class Poller;
class TimerId;
class TimerQueue;

class EventLoop{
public:
    using functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void runInLoop(functor cb);
    void queueInLoop(functor cb);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    
    bool isInLoopThread() const {return threadId_ == std::this_thread::get_id();}
    void assertInLoopThread() const;

    void wakeup();

    TimerId runAt(Timestamp time, functor cb);
    TimerId runAfter(double delay, functor cb);
    TimerId runEvery(double interval, functor cb);
    

private:
    void handleRead();
    void doPendingFunctors();

private:
    using ChannelList = std::vector<Channel*>;

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::atomic<bool> callingPendingFunctors_;

    std::unique_ptr<Poller> poller_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;

    std::mutex mutex_;
    std::vector<functor> pendingFunctors_;

    const std::thread::id threadId_;

    std::unique_ptr<TimerQueue> timerQueue_;

};
}
