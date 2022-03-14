#ifndef MYMUDUO_EPOLLPOLLER
#define MYMUDUO_EPOLLPOLLER

#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

namespace mymuduo
{

/*
epoll的使用
epoll_create   ==> 构造函数
epoll_ctl    add/mod/del  ==> update/remove
epoll_wait  ==> poll
*/


class EPollPoller : public mymuduo::Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activateChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
    
private:
    // Eventlist的初始长度
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation, Channel *channel);

    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;
};

}

#endif