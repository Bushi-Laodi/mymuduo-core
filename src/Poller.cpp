#include "mymuduo/Poller.h"
#include "mymuduo/Channel.h"
#include "mymuduo/EPollPoller.h"

namespace mymuduo{

bool Poller::hasChannel(Channel* channel) const{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

Poller* Poller::newDefaultPoller(EventLoop* loop){
    return new EPollPoller(loop);
}

}