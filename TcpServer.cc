
#include "TcpServer.h"
#include "Logger.h"
#include "util.h"


namespace mymuduo
{


TcpServer::TcpServer(EventLoop *loop,
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    // 当有新用户连接时，neConnection作为回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, 
                                        std::placeholders::_1, std::placeholders::_2));
    
}
     

TcpServer::~TcpServer()
{
    for(auto& item : connections_)
    {
        //这个智能指针对象暂时加一个引用，不至于让Conn对象直接消亡
        TcpConnectionPtr conn(item.second);
        // 释放Map中强智能指针的强绑定
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

//开启服务器监听
void TcpServer::start()
{
    if(started_++ == 0) //防止一个TcpServer对象被启动多次
    {
        threadPool_->start(threadInitCallback_);    //启动底层的线程池
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 有一个新的客户端的连接，acceptor会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询，选择一个subLoop
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
            name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获得其绑定的Addr
    InetAddress localAddr(getLocalAddr(sockfd));

    // 创建TcpConnection
    TcpConnectionPtr conn(new TcpConnection(ioLoop, 
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    //下面的回调都是用户设置的 TcpServer=>TcpConnection
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置了如何关闭连接的回调,shutdown=>shutdownInloop=>Channel::closeCallback=>TcpConnecitoin::handleClose=>Tcpserver::removeConnection
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
    
}
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
// 在loop中移除，不会发生多线程的错误
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}



}