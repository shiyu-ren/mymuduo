#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <mymuduo/Buffer.h>
#include <string>
#include <iostream>
#include <unistd.h>

using namespace mymuduo;

class EchoServer
{

public:
    EchoServer(EventLoop* loop, 
                const InetAddress& addr,
                const std::string& name)
        : server_(loop, addr, name),
          loop_(loop)
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)    
        );
        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:

    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("conn UP : %s", conn->peerAddress().toIpPort().c_str());
            ::sleep(5);
            conn->send("test sigpip");
        }
        else
        {
            LOG_INFO("conn DOWN : %s", conn->peerAddress().toIpPort().c_str());
            conn->shutdown();
        }

    }

    void onMessage(const TcpConnectionPtr& conn,
                    Buffer *buffer, 
                    Timestamp time)
    {
        std::string msg = buffer->retrieveAllAsString();
        conn->send(msg);
        // conn->shutdown();
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 8000);
    EchoServer server(&loop, addr, "EchoServer-01");
    server.start();
    loop.loop();

    return 0;
}   