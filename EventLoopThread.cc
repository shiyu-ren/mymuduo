


#include "EventLoopThread.h"
#include "EventLoop.h"

namespace mymuduo
{


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                const std::string& name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    // 启动底层的新线程，执行当前的threadfunc
    thread_.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 此方法在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    // 创建一个独立的eventloop，和上面的线程是一一对应的，one loop per thread
    EventLoop loop;

    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();    // Event loop => Poller.poll

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}


}