#include "mymuduo/EventLoop.h"
#include "mymuduo/Channel.h"
#include "mymuduo/Poller.h"
#include <stdexcept>
#include <unistd.h>
#include <sys/eventfd.h>
#include "mymuduo/TimerQueue.h"
#include "mymuduo/Timer.h"

namespace mymuduo{

thread_local EventLoop* t_LoopInThisThread = nullptr;//一个线程一个loop
const int kPollerTimeMs = 10000;

static int createEventFd(){
    int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(fd < 0) throw std::runtime_error("eventfd create error");
    return fd;
}

EventLoop::EventLoop()
    :looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(std::this_thread::get_id()),
    poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventFd()), wakeupChannel_(new Channel(this, wakeupFd_)),
    timerQueue_(new TimerQueue(this))
{
    if(t_LoopInThisThread) throw std::runtime_error("one thread cannot create two EventLoop objects");
    t_LoopInThisThread = this;
    wakeupChannel_->setReadCallback([this](Timestamp){ handleRead(); });
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_LoopInThisThread = nullptr;
}

void EventLoop::loop(){
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    while(!quit_){
        activeChannels_.clear();
        Timestamp pollerReturnTime = poller_->poll(kPollerTimeMs, &activeChannels_);
        for(auto channel : activeChannels_){
            channel->handleEvent(pollerReturnTime);
        }
        doPendingFunctors();
    }
    looping_ = false;
}

void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()) wakeup();
}

void EventLoop::runInLoop(functor cb){
    if(isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(functor cb){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if(!isInLoopThread() || callingPendingFunctors_){//有点问题
        wakeup();
    }
}

void EventLoop::wakeup(){//来唤醒Loop
    uint64_t one = 1;
    write(wakeupFd_, &one, sizeof(one));
}

void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

void EventLoop::assertInLoopThread() const{
    if(!isInLoopThread()){
        throw std::runtime_error("called from wrong EventLoop thread");
    }
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    read(wakeupFd_, &one, sizeof(one));
}

void EventLoop::doPendingFunctors(){
    std::vector<functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(auto func : functors){
        func();
    }
    callingPendingFunctors_ = false;
}

TimerId EventLoop::runAt(Timestamp time, functor cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0);
}

TimerId EventLoop::runAfter(double delay, functor cb)
{
    return runAt(Timer::addTime(Timestamp::now(), delay), std::move(cb));
}

TimerId EventLoop::runEvery(double interval, functor cb)
{
    return timerQueue_->addTimer(std::move(cb), Timer::addTime(Timestamp::now(), interval), interval);
}

}
