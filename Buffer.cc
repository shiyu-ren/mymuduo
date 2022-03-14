#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"

namespace mymuduo
{

const char Buffer::kCRLF[] = "\r\n";

// 从fd上读取数据  Poller工作在LT模式
//buffer缓冲区是有大小的，但是从fd上读数据时候，却不知道tcp数据最终的大小
//
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536];   //栈上的内存空间  64k
    struct iovec vec[2];
    const size_t writable = writableBytes(); //这是Buffer底层缓冲区剩余可写大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;   // 一次最多读64k

    const ssize_t n = ::readv(fd, vec, iovcnt);

    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writable)  //buffer的可写缓冲区够用
    {
        writerIndex_ += n;
    }
    else    //extrabuf中也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); //writeIndex_开始写n-writable大小的数据
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}


}