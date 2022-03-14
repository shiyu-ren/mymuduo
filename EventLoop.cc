
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <memory>
#include <signal.h>

#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include "TimerQueue.h"

namespace mymuduo
{


// 防止一个线程创建多个EventLoop   thread_local
__thread EventLoop* t_loopInThisThread = 0;

// 定义默认的Poller  IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subLoop处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}


class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    //设置wakeupfd的事件类型，以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this)
    );
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();

}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

//开启事件循环
void EventLoop::loop()
{
    assert(!looping_);
    // assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        //监听两种fd， 一种是client的fd，一种是wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        
        eventHandling_ = true;
        for(Channel* channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知Channel处理相应的事件
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        //执行当前EventLoop事件循环需要处理的回调操作
        /*
          IO线程 mainloop accept fd 《= channel subloop
          mainLoop实现注册一个回调cb（需要subloop来执行） wakeup subloop后，执行下面的方法，执行mainloop注册的cb
        */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环 1.loop在自己的线程中调用quit 2.在非loop的线程中，调用loop的quit
/*
            mainloop

    subloop1   subloop2   subloop3
*/
void EventLoop::quit()
{
    quit_ = true;

    //如果是在其他线程中调用quit，如在一个subloop（worker）中，调用了mainloop（IO）的quit
    if(!isInLoopThread())   
    {
        wakeup();
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())    // 在当前的loop线程中执行cb
    {
        cb();
    }
    else    // 在非当前loop线程执行cb,需要唤醒loop所在线程执行cb
    {
        queueInLoop(cb);
    }
}
// 把cb放入队列，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    //唤醒响应的，需要执行上面回调操作的loop的线程
    //|| callingPendingFunctors_的意思是： 当前loop正在执行回调，但是loop又有了新的回调，为防止结束回调后又阻塞在poll
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); //唤醒loop所在线程
    }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one))
  {
      LOG_ERROR("EventLoop::handleRead() reads %lu bytes \n", n);
  }
}

// 用来唤醒loop所在线程  向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop wakeup write %lu bytes instead of 8 \n", n);
    }
}

//EventLoop的方法 =》 Poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);

}
bool EventLoop::hasChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    poller_->hasChannel(channel);
}
   //执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    // 把functors转移到局部的functors，这样在执行回调时不用加锁。不影响mainloop注册回调
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors)
    {
        functor();  //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}

/******timers********/
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0.0);    
}


TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}
void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}



}