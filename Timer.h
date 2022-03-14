#ifndef MYMUDUO_TIMER
#define MYMUDUO_TIMER

#include <atomic>

#include "noncopyable.h"
#include "Callbacks.h"
#include "Timestamp.h"

namespace mymuduo
{

class Timer
{
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequece_(++s_numCreated_)
    {}

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequece() const { return sequece_; }

    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_; }

private:
    
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequece_;

    static std::atomic_int64_t s_numCreated_;
};



}


#endif