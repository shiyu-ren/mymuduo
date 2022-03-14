#ifndef MYMUDUO_SOCKET
#define MYMUDUO_SOCKET


#include "noncopyable.h"

namespace mymuduo
{

class InetAddress;

class Socket : public noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress& localAddr);
    void listen();
    int accept(InetAddress *peerAddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};



} // namespace mymuduo



#endif