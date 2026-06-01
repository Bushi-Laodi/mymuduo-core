#pragma oce
#include "mymuduo/Poller.h"
#include <sys/epoll.h>


namespace mymuduo{

    // 匿名命名空间，这些名字只在当前 .cpp 文件内部可见
    // 好处： 1、避免污染全局命名空间 2、避免和其他文件里的同名变量冲突
namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

class EventLoop;
class EPollPoller : public Poller{
public:
    explicit EPollPoller(EventLoop* loop);

    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    static const int kInitEventListSize = 16;
    int epollfd_;
    std::vector<epoll_event> events_;
};
}