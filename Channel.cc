#include <sys/epoll.h>
#include <cassert>

#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"

using namespace mymuduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop : ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    :loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    tied_(false),
    eventHandling_(false),
    addedToLoop_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if(loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));
    }
}

// 注意什么时候调用(新连接创建的时候) TcpConnection=>Channel
void Channel::tie(const std::shared_ptr<void> &obj)
{
    //tie_拥有TcpConnection的弱指针引用
    tie_ = obj;
    tied_ = true;
}

//当改变Channel所管理的fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
    addedToLoop_ = true;
    //通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的loop中，将当前的channel删除掉
void Channel::remove()
{
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard;
        guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件，执行回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d", revents_);
    eventHandling_ = true;
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)  closeCallback_();
        
    }
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)  errorCallback_();

    }
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)   readCallback_(receiveTime);
    }
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)  writeCallback_();
    }
    eventHandling_ = false;
}