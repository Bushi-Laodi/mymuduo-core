#include "mymuduo/EventLoopThreadPool.h"
#include "mymuduo/EventLoopThread.h"
#include "mymuduo/EventLoop.h"
#include <cstdio>

namespace mymuduo {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
    : baseLoop_(baseLoop), name_(name), started_(false), numThreads_(0), next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_ = true;
    for (int i = 0; i < numThreads_; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s-%d", name_.c_str(), i);
        std::unique_ptr<EventLoopThread> t(new EventLoopThread(cb, buf));
        loops_.push_back(t->startLoop());
        threads_.push_back(std::move(t));
    }
    if (numThreads_ == 0 && cb) cb(baseLoop_);
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    EventLoop* loop = baseLoop_;
    if (!loops_.empty()) {
        loop = loops_[next_++];
        if (static_cast<size_t>(next_) >= loops_.size()) next_ = 0;
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) return std::vector<EventLoop*>(1, baseLoop_);
    return loops_;
}

}