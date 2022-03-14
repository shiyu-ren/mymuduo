#pragma once

#include "noncopyable.h"
#include <string>

namespace mymuduo
{


// 时间类
class Timestamp 
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    ~Timestamp();

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    
    static Timestamp now();
    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    void swap(Timestamp& that)
    { std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_); }

    bool valid() const { return microSecondsSinceEpoch_ > 0; }


    static Timestamp invalid()    { return Timestamp(); }

    static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator< (const Timestamp& lhs, const Timestamp& rhs)
{ return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch(); }


inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}