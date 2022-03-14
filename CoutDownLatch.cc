#include "CountDownLatch.h"

namespace mymuduo
{

CountDownLatch::CountDownLatch(int count)
    : mutex_(),
      condition_(),
      count_(count)
{
}
void CountDownLatch::CountDownLatch::wait()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while(count_ > 0)
    {
        condition_.wait(lock);
    }
}
void CountDownLatch::countDown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    --count_;
    if(count_ == 0) condition_.notify_all();
}

int CountDownLatch::getCount()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
}


}