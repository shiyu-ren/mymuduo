#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

namespace mymuduo
{

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : start_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name),
      latch_(1)
{
    setDefaultName();
}
Thread::~Thread()
{
    if(start_ && !joined_)
    {
        thread_->detach();  // thread类提供的设置分离线程的方法
    }
}

// 一个Thread对象，记录的就是一个线程的详细信息
void Thread::start()
{
    start_ = true;
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        //获取线程的tid值
        tid_ = CurrentThread::tid();
        //使用CountDownLatch到时器进行同步，使得这个tid_获得后，主线程才可执行
        latch_.countDown();
        func_();    // 开启一个新线程，专门执行该线程函数
    }));

    // 这里必须等待获取上面新创建的线程的tid值
    latch_.wait();
}

int Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}


}