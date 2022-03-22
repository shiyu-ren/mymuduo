### 使用C++动手实现自己的网络库(参考Muduo)
#### 简介
本项目是参考陈硕老师的Muduo网络库，与Muduo的不同点：
* 去掉了Muduo库中的Boost依赖，完全使用C++标准，如使用std::function<>
* 没有单独封装Thread，使用C++11引入的std::thread搭配lambda表达式实现工作线程，没有直接使用pthread库。类似的直接使用C++11/17的还有std::atomic，std::any等
* 只实现了epoll这一个IO-Multiplexing,没有实现poll/select
* 日志系统暂时还没有实现Muduo的双缓冲异步日志系统，制作了简单的分级日志输出
* Buffer部分Muduo库没有提供writeFd方法，本项目加入了writeFd，在处理outputBuffer剩余未发数据时交给Buffer来处理
* 暂时没有做TcpClient部分

具体实现见博文
#### 安装使用
进入项目根目录，创建build文件夹进行编译
```
mkdir build
cd build
cmake ..
make
```
回到项目根目录，将头文件复制到/usr/include/mymuduo
```
cp *.h /usr/include/mymuduo
```
进入lib文件夹，将库复制到/usr/local/lib
```
cd lib
cp libmymuduo.so /usr/local/lib
```

#### 编程示例
使用时编程风格与使用muduo一样，以下展示一个简单的echoserver,具体代码见example

定义EchoServer类
```c++
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

    void setThreadNum(int threadNum)
    {
        server_.setThreadNum(threadNum);
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
    }

    EventLoop *loop_;
    TcpServer server_;
};

```
在main函数中运行EchoServer 
```c++
int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 8888);
    EchoServer server(&loop, addr, "EchoServer-01");
    server.start();
    loop.loop();

    return 0;
}   
```

可以使用netcat进行测试
```
nc localhost 8888
```

#### 网络库测压
在example中使用mymuduo实现了http服务器（只是简单的通过/hello返回hello world），使用apache benchmark对此http服务器进行压力测试，可以测试网络库的并发处理能力
在本人笔记本上（CPU i5-6200U）使用ab进行测压（setThreadNums = 4）
```
ab -n 1000000 -c 1000 -k http://127.0.0.1:8000/hello
```
数据为
```
Concurrency Level:      1000
Time taken for tests:   48.376 seconds
Complete requests:      1000000
Failed requests:        0
Keep-Alive requests:    1000000
Total transferred:      118000000 bytes
HTML transferred:       14000000 bytes
Requests per second:    20671.60 [#/sec] (mean)
Time per request:       48.376 [ms] (mean)
Time per request:       0.048 [ms] (mean, across all concurrent requests)
Transfer rate:          2382.08 [Kbytes/sec] received
```
若减少在updateChannel以及handleEvent时候的Log输出与http服务器对发送数据的打印，此时性能为
```
Concurrency Level:      1000
Time taken for tests:   14.852 seconds
Complete requests:      1000000
Failed requests:        0
Keep-Alive requests:    1000000
Total transferred:      118000000 bytes
HTML transferred:       14000000 bytes
Requests per second:    67330.24 [#/sec] (mean)
Time per request:       14.852 [ms] (mean)
Time per request:       0.015 [ms] (mean, across all concurrent requests)
Transfer rate:          7758.76 [Kbytes/sec] received
```
可以看出在并发处理数据的速度还是很快速的
**与nginx对比**
使用openresty集成的nginx http服务器作为对比(worker_processes=4)
nginx的conf文件为
```
#user  nobody;
worker_processes  4;
events {
    worker_connections  10240;
}
http {
    include       /usr/local/openresty/nginx/conf/mime.types;
    default_type  application/octet-stream;
    access_log  off;
    sendfile       on;
    tcp_nopush     on;
    keepalive_timeout  65;
    server {
        listen       8001;
        server_name  localhost;
        location / {
            root   html;
            index  index.html index.html;
        }
        location /hello {
          default_type text/plain;
          echo "hello, world!";
        }
    }
}
```
使用同样需求量与并发连接量的ab测压，数据为
```
Concurrency Level:      1000
Time taken for tests:   56.401 seconds
Complete requests:      1000000
Failed requests:        0
Keep-Alive requests:    0
Total transferred:      143000000 bytes
HTML transferred:       14000000 bytes
Requests per second:    17730.13 [#/sec] (mean)
Time per request:       56.401 [ms] (mean)
Time per request:       0.056 [ms] (mean, across all concurrent requests)
Transfer rate:          2475.99 [Kbytes/sec] received

```
可以看出在处理速度上还是比nginx要好
