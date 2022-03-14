#pragma once

#include <functional>
#include <string>
#include <memory>

#include "EventLoop.h"
#include "noncopyable.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "Callbacks.h"

namespace mymuduo
{

/*
用户使用muduo编写服务器程序
*/

// 对外使用的TcpServer
class TcpServer
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
                const InetAddress& listenAddr,
                const std::string& nameArg,
                Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    //设置底层subloop的个数
    void setThreadNum(int numThreads);

    //开启服务器监听
    void start();

    std::string name() const { return name_; }
    std::string ipPort() const { return ipPort_; }
    EventLoop* getLoop() const { return loop_; }

private:

    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    // 在loop中移除，不会发生多线程的错误
    void removeConnectionInLoop(const TcpConnectionPtr& conn); 

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;    //用户定义的baseloop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;    //运行在mainloop， 任务是监听连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_;   // one loop per thread

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_;   //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   //消息发送完成以后的回调

    ThreadInitCallback threadInitCallback_; //线程初始化回调
    std::atomic_int32_t started_;

    int nextConnId_;
    ConnectionMap connections_; //保存所有的连接
};



}