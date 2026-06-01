#pragma once
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace mymuduo {

class EventLoop;

class EventLoopThread {// 创建一个thread来跑EventLoop
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string& name = "");
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
    std::string name_;
};

}