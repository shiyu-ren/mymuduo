#ifndef MYMUDUO_TIMERQUEUE
#define MYMUDUO_TIMERQUEUE

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "noncopyable.h"
#include "Callbacks.h"
#include "Channel.h"
#include "Timestamp.h"

namespace mymuduo
{

class Timer;
class TimerId;

class TimerQueue : public noncopyable
{

public:


    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // TimerQueue提供给loop的添加关闭定时器的接口函数。 必须是线程安全的（往往都在其他线程中被调用）
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

private:
    using TimerPtr = Timer*;
    using Entry = std::pair<Timestamp, TimerPtr>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<TimerPtr, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    // 保证线程安全的操作
    void addTimerInLoop(TimerPtr timer);
    void cancelInLoop(TimerId timerId);

    //当timerfd事件就绪时执行
    void handleRead();

    //将所有超时的timer移除
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(TimerPtr timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    // 存储了Timer， 使用set容器（红黑树），按照Expiration时间进行排序
    TimerList timers_;

    ActiveTimerSet activeTimers_;
    bool callingExpiredTimers_;
    ActiveTimerSet cancelingTimers_;

};

}
#endif