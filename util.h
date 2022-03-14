#ifndef MYMUDUO_UTIL
#define MYMUDUO_UTIL

#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>

#include "EventLoop.h"
#include "Logger.h"

namespace mymuduo
{


inline EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("MainLoop is null!\n");
    }
    return loop;
}

inline struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, (sockaddr*)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr\n");
    }
    return localaddr;
}

inline struct sockaddr_in getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, (sockaddr*)(&peeraddr), &addrlen) < 0)
    {
        LOG_ERROR("sockets::getPeerAddr\n");
    }
    return peeraddr;
}

}

#endif
