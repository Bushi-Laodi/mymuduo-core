#include "mymuduo/TimerQueue.h"
#include "mymuduo/EventLoop.h"
#include <cstring>
#include <stdint.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace mymuduo {

static int createTimerfd() {
    return ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);//创建可以被epoll监听的定时器描述符
}

// 转换成系统调用的时间输入
static timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) microseconds = 100;
    timespec ts;
    /*
    struct timespec {
    time_t tv_sec;   // 秒
    long   tv_nsec;  // 纳秒
    }; 
    */
    ts.tv_sec = static_cast<time_t>(microseconds / 1000000);
    ts.tv_nsec = static_cast<long>((microseconds % 1000000) * 1000);
    return ts;
}

static void resetTimerfd(int timerfd, Timestamp expiration) {
    itimerspec newValue;
    /*
    struct itimerspec {
    struct timespec it_interval;  // 重复间隔
    struct timespec it_value;     // 第一次触发时间
    };  
    */
    std::memset(&newValue, 0, sizeof(newValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    ::timerfd_settime(timerfd, 0, &newValue, nullptr);
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_) {
    timerfdChannel_.setReadCallback([this](Timestamp) { handleRead(); });
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for (const Entry& e : timers_) delete e.second;//删除所有的定时器
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop([this, timer] { addTimerInLoop(timer); });
    return TimerId(timer);
}

// 这个需要进行判断加入当前的定时器后是否需要对timerfd进行更新
void TimerQueue::addTimerInLoop(Timer* timer) {
    bool earliestChanged = insert(timer);//如果比当前存储的所有的定时器都短那么就要更新timerfd
    if (earliestChanged) resetTimerfd(timerfd_, timer->expiration());
}

//真正的添加函数
bool TimerQueue::insert(Timer* timer) {
    bool earliestChanged = timers_.empty() || timer->expiration() < timers_.begin()->first;
    timers_.insert(Entry(timer->expiration(), timer));
    return earliestChanged;
}

void TimerQueue::handleRead() {
    uint64_t exp = 0;
    ::read(timerfd_, &exp, sizeof(exp));//读一下timerfd_，避免epoll一直报可读
    Timestamp now = Timestamp::now();//获取当前时间
    std::vector<Entry> expired = getExpired(now);//获取到期的定时器
    for (const Entry& it : expired) it.second->run();
    reset(expired, now);
}

// 获取到期的的定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));//UINTPTR_MAX表示最大指针
    auto end = timers_.lower_bound(sentry);//找边界，找到第一个不小于 x 的元素
    std::copy(timers_.begin(), end, back_inserter(expired));//把 timers_.begin() 到 end 这一段元素，复制到 expired 这个 vector 的末尾。
    timers_.erase(timers_.begin(), end);
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    for (const Entry& it : expired) {
        Timer* timer = it.second;
        if (timer->repeat()) {
            timer->restart(now);
            insert(timer);
        } else {
            delete timer;
        }
    }
    if (!timers_.empty()) resetTimerfd(timerfd_, timers_.begin()->second->expiration());
}

}