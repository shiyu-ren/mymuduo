
#ifndef MYMUDUO_BUFFER
#define MYMUDUO_BUFFER

#include <vector>
#include <string>
#include <algorithm>

namespace mymuduo
{


//网络层底层的缓冲定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const
    { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const
    { return buffer_.size() - writerIndex_; }

    size_t prependableByte() const
    { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    { return begin() + readerIndex_; }

    // OnMessage string <- Buffer
    void retrieve(size_t len)
    {
        if(len <= readableBytes())
        {
            readerIndex_ += len;    // 应用只读取了可读缓冲区的一部分（len）
        }
        else  
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把OnMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);  // 上面一句把缓冲区中可读的数据已经读取出来，这里对缓冲区进行
        return result;
    }

    // buffer_.size - writerIndex_   len
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len); //扩容函数
        }
    }

    void append(const std::string& str)
    { append(str.c_str(), str.size()); }

    // 把 [data,data+len]内存上的数据加到writable缓冲区当中
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    void retrieveUntil(const char* end)
    {
        retrieve(end - peek());
    }

    char* beginWrite()
    { return begin() + writerIndex_; }
    const char* beginWrite() const
    { return begin() + writerIndex_; }

    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const
    { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableByte() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                        begin() + writerIndex_, 
                        begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];

};

}


#endif 