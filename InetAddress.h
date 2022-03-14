#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址类型

namespace mymuduo
{

class InetAddress
{

public:
    //本地环回
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);
    // ip与port构造
    InetAddress(const std::string& ip, uint16_t port);
    //sockaddr构造
    explicit InetAddress(const struct sockaddr_in& addr)
    : addr_(addr)
    { }

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const struct sockaddr* getSockAddr() const
    {   return reinterpret_cast<const struct sockaddr*>(&addr_); }

    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; };

    // socklen_t getSockLen() const
    // {   return }

private:
        struct sockaddr_in addr_;
};



}