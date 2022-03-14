#ifndef MYMUDUO_EVENTLOOPTHREADPOOL
#define MYMUDUO_EVENTLOOPTHREADPOOL

#include <functional>
#include <vector>
#include <memory>

#include "noncopyable.h"

namespace mymuduo
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable
{

public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop, 
                        const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 在多线程中，baseLoop会默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();
    //使用hash的方式分配loop 
    EventLoop* getLoopForHash(size_t hashCode);

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }

    const std::string name() const { return name_; }

private:

    EventLoop *baseLoop_;   // 用户创建的 EventLoop loop;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;

};

}


#endif