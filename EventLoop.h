#ifndef MYMUDUO_EVENTLOOP
#define MYMUDUO_EVENTLOOP

#include <vector>
#include <atomic>
#include <memory>
#include <any>
#include <mutex>
#include <functional>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "Poller.h"
#include "Logger.h"
#include "TimerId.h"
#include "Callbacks.h"

namespace mymuduo
{

class TimerQueue;

// 事件循环类，主要包含了两大模块  Channel 与Poller（还有定时器事件）
class EventLoop : public noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    //在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeup();

    //EventLoop的方法 =》 Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断EventLoop对象是否在自己的线程里边
    bool isInLoopThread() const 
    { return threadId_ == CurrentThread::tid(); }

    bool eventHandling() const { return eventHandling_; }

    void setContext(const std::any& context)
    { context_ = context; }

    const std::any& getContext() const
    { return context_; }

    std::any* getMutableContext()
    { return &context_; }

    /******timers********/
    TimerId runAt(Timestamp time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double interval, TimerCallback cb);
    void cancel(TimerId timerId);

private:
    using ChannelList = std::vector<Channel*>;
    
    void handleRead();  //wake up
    void doPendingFunctors();   //执行回调

    void printActiveChannels() const;

    std::atomic_bool looping_;  //原子操作，通过CAS实现的
    std::atomic_bool quit_; // 标志退出loop循环
    std::atomic_bool eventHandling_;    // 表示是否在处理事件
    
    const pid_t threadId_;  //记录当前线loop所在的线程
    Timestamp pollReturnTime_; //poller返回发生事件的channels的时间点

    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_; // 当mainLoop获取一个新用户的fd，通过轮询算法选择一个subLoop，通过该成员唤醒subLoop处理（eventfd）
    std::unique_ptr<Channel> wakeupChannel_;

    std::any context_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_;   //标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;  //存储loop需要执行的所有回调操作
    std::mutex mutex_;  // 互斥锁，用来保护上面vector容器的线程安全操作
};


}

#endif