#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

using namespace mymuduo;

// LOG_INFO("%s  %d", arg1, arg2)

    //  获取日志单例
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
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    
    case ERROR:
        std::cout << "[INFO]";
        break;
    case FATAL:
        std::cout << "[INFO]";
        break;
    case DEBUG:
        std::cout << "[INFO]";
        break;
    default:
        break;
    }

    std::cout << "print time : " << Timestamp::now().toFormattedString(false) <<  msg << std::endl;
}
