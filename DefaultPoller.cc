
// 单独写一个.cc文件实现Poller中的静态方法
// 因为这个方法要生成一个Poller的派生类的对象
// 如果实现在Poller中，需要包含Poller派生类头文件，不符合代码规范，容易出错
#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

using namespace mymuduo;

// Eventloop可以通过该接口获取默认的IO复用的具体对象
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop);
    }
}
