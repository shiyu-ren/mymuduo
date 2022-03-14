
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>  // snprintf
#include <sys/socket.h>
#include <sys/uio.h>  // readv
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/tcp.h>


#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

namespace mymuduo
{

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localAddr)
{
    if(::bind(sockfd_, localAddr.getSockAddr(), sizeof(*localAddr.getSockAddr())) != 0)
    {
        LOG_FATAL("Socket bind sockfd error! errno=%d\n", errno);
    }
}


void Socket::listen()
{
    if(::listen(sockfd_, 1024) != 0)
    {
        LOG_FATAL("Socket listen sockfd error! errno=%d\n", errno);
    }
}
int Socket::accept(InetAddress *peerAddr)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0)
    {
        peerAddr->setSockAddr(addr);
    }
    else
    {
        LOG_FATAL("Socket accept sockfd error! errno=%d\n", errno);
    }

    return connfd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("Socket shutdownwrite sockfd error! errno=%d\n", errno);
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}



}