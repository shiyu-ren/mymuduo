
#include <cassert>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

using namespace mymuduo;

//channel未添加到poller中
const int kNew = -1;    //channel的成员index = -1
//channel已添加
const int kAdded = 1;
//channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    :Poller(loop),
     epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
     events_(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("EPollPoller epoll_create error! errno=%d\n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// channel update => EventLoop updateChannel => Poller updateChannel
/*
        EventLoop
    ChannelList     Poller
                    ChannelMap <fd, channel*>
*/
void EPollPoller::updateChannel(Channel *channel)
{
    // Poller::assertInLoopThread();
    const int index = channel->index();
    LOG_INFO("func=%s, fd=%d, event=%d, index=%d \n",__FUNCTION__, channel->fd(), channel->events(), index);
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else    //已经注册过
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    // LOG_INFO("func=%d, fd=%d, event=%d, index=%d \n",__FUNCTION__, channel->fd(), channel->events(), index);
    int fd = channel->fd();
    channels_.erase(fd);
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activateChannels)
{
    // 应当使用LOG_DEBUG
    LOG_INFO("poll called\n");

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), 
                            static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activateChannels);
        // 可能不够，扩容等下次epoll_wait（由于是LT模式，没读得会继续上报）
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller poll error\n");
        }
    }
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); //EventLoop就拿到了Poller给他返回的所有就绪的fd了
    }
}

// 更新channel通道
void EPollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}