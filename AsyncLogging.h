#ifndef MYMUDUO_ASYNCLOGGING
#define MYMUDUO_ASYNCLOGGING

#include "CountDownLatch.h"
#include "noncopyable.h"
#include "Thread.h"
#include "Logger.h"

#include <atomic>
#include <vector>

namespace mymuduo
{

class AsyncLogging : noncopyable
{
public:

    AsyncLogging(const std::string& basename,
                off_t rollSize,
                int flushInterval = 3);

    ~AsyncLogging()
    {
    if (running_)
    {
        stop();
    }
    }

    void append(const char* logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() 
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:

    void threadFunc();

    // typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
    using Buffer = FixedBuffer<mymuduo::kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    CountDownLatch latch_;
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

}  // namespace muduo

#endif  // MUDUO_BASE_ASYNCLOGGING_H
