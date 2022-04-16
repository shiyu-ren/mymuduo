#ifndef  MYMUDUO_LOGGER
#define MYMUDUO_LOGGER

#include <string>
#include <cstring>

#include "noncopyable.h"

namespace mymuduo
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000;

template<int SIZE>
class FixedBuffer : noncopyable
{
 public:
  FixedBuffer()
    : cur_(data_)
  {
    // setCookie(cookieStart);
  }

  ~FixedBuffer()
  {
    // setCookie(cookieEnd);
  }

  void append(const char* /*restrict*/ buf, size_t len)
  {
    if (static_cast<size_t>(avail()) > len)
    {
      ::memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  // write to data_ directly
  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { ::bzero(data_, sizeof data_); }

  // for used by GDB
  const char* debugString();
//   void setCookie(void (*cookie)()) { cookie_ = cookie; }
  // for used by unit test
  std::string toString() const { return std::string(data_, length()); }

 private:
  const char* end() const { return data_ + sizeof data_; }
  // Must be outline function for cookies.
//   static void cookieStart();
//   static void cookieEnd();

//   void (*cookie_)();
  char data_[SIZE];
  char* cur_;
};



//定义日志的级别   INFO  ERROR   FATAL   DEBUG
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};

//   日志类

class Logger : noncopyable
{
public:

    using LogBuffer = FixedBuffer<kSmallBuffer>;

    //  获取日志单例
    static Logger& GetInstance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
    using OutputFunc = void (*)(const char*, int);
    using FlushFunc = void (*)();
    static void setOutPut(OutputFunc);
    static void setFlush(FlushFunc);

private:
    int logLevel_;
    Logger(){}
};


#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::GetInstance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::GetInstance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::GetInstance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MYMUDUO_DEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif




}

#endif