#ifndef MYMUDUO_POLLER
#define MYMUDUO_POLLER

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

namespace mymuduo
{

class EventLoop;
class Channel;

// muduo库中多路事件分发器的核心IO复用模块
class Poller : public noncopyable
{

public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用（epoll/select）保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activateChannels_) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    
    //判断channel是否在当前Poller当中
    virtual bool hasChannel(Channel *channel) const;

    // Eventloop可以通过该接口获取默认的IO复用的具体对象
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // map的key表示sockfd， value是fd所对应的channel对象
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;  // Poller所属的Loop

};


}

#endif