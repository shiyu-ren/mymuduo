#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

using namespace mymuduo;

// LOG_INFO("%s  %d", arg1, arg2)

    //  获取日志单例

namespace mymuduo
{

void defaultOutput(const char* msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    
    (void)n;
}

void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;


}

Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}
// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
// 写日志
void Logger::log(std::string msg)
{
    std::string buf = "";
    switch (logLevel_)
    {
    case INFO:
        // std::cout << "[INFO]";
        buf += "[INFO]";
        break;
    
    case ERROR:
        // std::cout << "[INFO]";
        buf += "[ERROR]";
        break;
    case FATAL:
        // std::cout << "[INFO]";
        buf += "[FATAL]";
        break;
    case DEBUG:
        // std::cout << "[INFO]";
        buf += "[DEBUG]";
        break;
    default:
        break;
    }
    buf += "Time:" + Timestamp::now().toFormattedString(false) + "Content:" + msg;

    g_output(buf.c_str(), buf.size());
    if(logLevel_ == FATAL)
    {
        g_flush();
        exit(0);
    }
    // std::cout << "print time : " << Timestamp::now().toFormattedString(false) <<  msg << std::endl;
}

void Logger::setOutPut(OutputFunc out)
{
    g_output = out;
}
void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}
