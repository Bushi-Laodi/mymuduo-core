#include "mymuduo/Channel.h"
#include "mymuduo/EventLoop.h"
#include <sys/epoll.h>

namespace mymuduo{
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;//普通读事件 和 高优先级读事件
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false){}

Channel::~Channel(){}

//进行生命周期的绑定
void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}

void Channel::enableReading() { events_ |= kReadEvent; update(); }
void Channel::disableReading() { events_ &= ~kReadEvent; update(); }
void Channel::enableWriting() { events_ |= kWriteEvent; update(); }
void Channel::disableWriting() { events_ &= ~kWriteEvent; update(); }
void Channel::disableAll() { events_ = kNoneEvent; update(); }

void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){//tied_表示这个 Channel 是否启用了生命周期保护，启用了就要guard
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{//未启用就直接调用回调函数
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime){
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){//如果发生了挂断，并且没有可读数据，才直接走 closeCallback_
        if(closeCallback_) closeCallback_();
    }
    if(revents_ & EPOLLERR){// 如果发生错误
        if(errorCallback_) errorCallback_();
    }
    if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLHUP)){//有可读事件 + 挂断状态的时候为读尽缓冲区
        if(readCallback_) readCallback_(receiveTime);
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_) writeCallback_();
    }
}

//所谓的update都是用来更新epoll的监听对象的，update是用来更新感兴趣的事件，remove是将想要监听的对象移除不在监听
//update/remove都应该在loop线程中执行，在Epoller中更新
void Channel::update(){
    loop_->updateChannel(this);
}

void Channel::remove(){
    loop_->removeChannel(this);
}

}