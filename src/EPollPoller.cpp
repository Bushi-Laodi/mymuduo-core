#include "mymuduo/EPollPoller.h"
#include "mymuduo/Channel.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <cerrno>

namespace mymuduo{

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),events_(kInitEventListSize){
        if(epollfd_ < 0) throw std::runtime_error("epoll_create1 False");
}

EPollPoller::~EPollPoller()  {
    close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    int numsEvents = epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    Timestamp now = Timestamp::now();
    if(numsEvents > 0){
        fillActiveChannels(numsEvents, activeChannels);
        if(numsEvents == static_cast<int>(events_.size())){
            events_.resize(numsEvents * 2);
        }
    }
    /*
    EBADF   epollfd 无效
    EFAULT  events 指针非法
    EINVAL  参数非法
    EINTR   被信号中断 --> 这不代表 epoll 出了严重错误，只是被别的信号打断，是可忽略的，下一次loop可以正常运行，
    */
   // 该判断是用来处理不可忽略的错误
    else if(numsEvents < 0 && errno != EINTR){
        throw std::runtime_error("epoll_wait error");
    }
    return now;
}

void EPollPoller::updateChannel(Channel* channel) {
    int index = channel->index();
    int fd = channel->fd();
    if(index == kDeleted || index == kNew){
        channels_[fd] = channel;
        update(EPOLL_CTL_ADD, channel);
        channel->set_index(kAdded);
    }
    else{
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);
    if(channel->index() == kAdded) update(EPOLL_CTL_DEL, channel);
    channel->set_index(kDeleted);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const{
    for(int i = 0; i < numEvents; i++){
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel* channel){
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = channel;
    event.events = channel->events();
    if(epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0){
        throw std::runtime_error("epoll_ctl false");
    }
}

}
