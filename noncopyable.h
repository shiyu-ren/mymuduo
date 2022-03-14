#ifndef  MYMUDUO_NONCOPYABLE
#define MYMUDUO_NONCOPYABLE

namespace mymuduo
{

/*
noncopyable被继承之后，派生类对象可以正常的构造和析构，但是派生类对象
无法进行拷贝构造和拷贝赋值操作

*/

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

}



#endif