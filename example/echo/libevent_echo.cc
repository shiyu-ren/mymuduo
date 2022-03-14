#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <event.h>
#include <queue>
#include <iostream>

#include <arpa/inet.h>


#include <memory.h>


const int thread_num = 4;
#define BUF_SIZE 1024

using namespace std;



typedef struct {
    pthread_t tid;
    struct event_base *base;
    struct event event;
    int read_fd;
    int write_fd;
    //queue<int> q;
    int f_connect;
    char * buffer;
}LIBEVENT_THREAD;                 //需要保存的信息结构，用于管道通信和基事件的管理





typedef struct {
    pthread_t tid;
    struct event_base *base;
}DISPATCHER_THREAD;



LIBEVENT_THREAD *threads = (LIBEVENT_THREAD *) calloc(thread_num, sizeof(LIBEVENT_THREAD));


void on_write(int sock, short event, void* arg);

void on_read(int sock, short event, void* arg)
{

    cout<<"on_read() called, sock="<<sock<<endl;
    if(NULL == arg){
        return;
    }
    LIBEVENT_THREAD* event_thread = (LIBEVENT_THREAD*) arg;//获取传进来的参数
    char* buffer = new char[BUF_SIZE];
    memset(buffer, 0, sizeof(char)*BUF_SIZE);
    //--本来应该用while一直循环，但由于用了libevent，只在可以读的时候才触发on_read(),故不必用while了
    int size = read(sock, buffer, BUF_SIZE);
    if(0 == size){//说明socket关闭
        cout<<"read size is 0 for socket:"<<sock<<endl;
      //  destroy_sock_ev(event_struct);

      //event_thread->q.pop();
       close(sock);
        return;
    }
    cout<<"i have receive: "<<buffer<<endl;



    event_thread->buffer=buffer;

    struct event* write_ev = (struct event*)malloc(sizeof(struct event));//发生写事件（也就是只要socket缓冲区可写）时，就将反馈数据通过socket写回客户端
    event_set(write_ev, sock, EV_WRITE, on_write, event_thread);

    event_base_set(event_thread->base, write_ev);
    event_add(write_ev, NULL);
    cout<<"on_read() finished, sock="<<sock<<endl;

}




void on_write(int sock, short event, void* arg)
{

    if(NULL == arg){
        return;
    }
    LIBEVENT_THREAD* event_write_thread = (LIBEVENT_THREAD*) arg;//获取传进来的参数
    //char* buffer = new char[BUF_SIZE];
    //memset(buffer, 0, sizeof(char)*BUF_SIZE);

    //strcpy(buffer,event_write_thread->buffer,sizeof(event_write_thread->buffer));
    //--本来应该用while一直循环，但由于用了libevent，只在可以读的时候才触发on_read(),故不必用while了
    write(sock, event_write_thread->buffer, BUF_SIZE);

    free(event_write_thread->buffer);

    event_write_thread->buffer=NULL;

}




static void thread_libevent_process(int fd, short which, void *arg)
{
    int ret;
    char buf[128];
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *) arg;

    int fdconnect;

    if (fd != me->read_fd) {
        printf("thread_libevent_process error : fd != me->read_fd\n");
        exit(1);
    }


            ret = read(fd, buf, 128);
           if (ret > 0) 
            {

              buf[ret] = '\0';

              printf("thread %llu receive message : %s\n", (unsigned long long)me->tid, buf);

            }


           cout<<"thread_libevent_process\n"<<endl;

    /*if(me->q.size()>0)
        {
            fdconnect=me->q.front();
            me->q.pop();

           ret = read(fd, buf, 128);
           if (ret > 0) 
            {

              buf[ret] = '\0';

              printf("thread %llu receive message : %s\n", (unsigned long long)me->tid, buf);

            }
        }*/

        /*if(me->q.size()>0)
        {
            fdconnect=me->q.front();


            cout<<"thread_libevent_process succeed "<<endl;
            //me->q.pop();
        }

        else
            return ;*/


        fdconnect=me->f_connect;

        struct event* read_ev = (struct event*)malloc(sizeof(struct event));//发生读事件后，从socket中取出数据

        event_set(read_ev, fdconnect, EV_READ|EV_PERSIST, on_read, me);

        event_base_set(me->base, read_ev);

        event_add(read_ev, NULL);


    return;
}






void thread_init()
{
    int ret;
    int fd[2];
    for (int i = 0; i < thread_num; i++) {

            ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);




            if (ret == -1) {
                perror("socketpair()");
                return  ;
            }

            threads[i].read_fd = fd[0];
            threads[i].write_fd = fd[1];

            threads[i].base = event_init();


            if (threads[i].base == NULL) {
                perror("event_init()");
                return ;
            }

            event_set(&threads[i].event,threads[i].read_fd, EV_READ | EV_PERSIST, thread_libevent_process, &threads[i]);



            event_base_set(threads[i].base, &threads[i].event);
            if (event_add(&threads[i].event, 0) == -1) {
                perror("event_add()");
                return ;
            }

            cout<<"thread_init succeed"<<endl;

        }
}


 void * worker_thread(void *arg)
{
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg;
    me->tid = pthread_self();

    //event_base_loop(me->base, 0);

     event_base_dispatch(me->base);


    return NULL;
}




void CreatPhreadPool()
{

    for (int i = 0; i < thread_num; i++) {
        pthread_create(&threads[i].tid, NULL, worker_thread, &threads[i]);
    }


    cout<<"CreatPhreadPool"<<endl;
}







int getSocket(){
    int fd =socket( AF_INET, SOCK_STREAM, 0 );
    if(-1 == fd){
        cout<<"Error, fd is -1"<<endl;
    }
    return fd;
}


int last_thread=0;

void event_handler(int sock, short event, void* arg)  //添加其他信息

{
    struct sockaddr_in remote_addr;
    int sin_size=sizeof(struct sockaddr_in);
    int new_fd = accept(sock,  (struct sockaddr*) &remote_addr, (socklen_t*)&sin_size);    //如果线程池已用完，怎么办呢？
    if(new_fd < 0){
        cout<<"Accept error in on_accept()"<<endl;
        return;
    }
    cout<<"new_fd accepted is "<<new_fd<<endl;

    int tid = (last_thread + 1) % thread_num;        //memcached中线程负载均衡算法

    LIBEVENT_THREAD *thread = threads + tid;

    last_thread = tid;  

    thread->f_connect=new_fd;

    write(thread->write_fd, " ", 1);

    cout<<"on_accept() finished for fd="<<new_fd<<endl;
}






DISPATCHER_THREAD dispatcher_thread;        //用于设置主线程的结构变量




int main(int argc, char** argv)  
{



     thread_init();


     CreatPhreadPool();






    int fd_listen = getSocket();
     if(fd_listen <0){
         cout<<"Error in main(), fd<0"<<endl;
     }
     //cout<<"main() fd="<<fd<<endl;
     //----为服务器主线程绑定ip和port------------------------------
     struct sockaddr_in local_addr; //服务器端网络地址结构体
     memset(&local_addr,0,sizeof(local_addr)); //数据初始化--清零
     local_addr.sin_family=AF_INET; //设置为IP通信
     local_addr.sin_addr.s_addr=inet_addr(argv[1]);//服务器IP地址
     local_addr.sin_port=htons(atoi(argv[2])); //服务器端口号
     int bind_result = bind(fd_listen, (struct sockaddr*) &local_addr, sizeof(struct sockaddr));
     if(bind_result < 0){
         cout<<"Bind Error in main()"<<endl;
         return -1;
     }
     cout<<"bind_result="<<bind_result<<endl;
     listen(fd_listen, 10);


      evutil_make_socket_nonblocking(fd_listen);
     //-----设置libevent事件，每当socket出现可读事件，就调用on_accept()------------
     struct event_base* base = event_base_new();
     dispatcher_thread.base=base;
     dispatcher_thread.tid = pthread_self();

     struct event listen_ev;
     event_set(&listen_ev, fd_listen, EV_READ|EV_PERSIST, event_handler, NULL);
     event_base_set(dispatcher_thread.base, &listen_ev);
     event_add(&listen_ev, NULL);
     event_base_dispatch(dispatcher_thread.base);
     //------以下语句理论上是不会走到的---------------------------
     cout<<"event_base_dispatch() in main() finished"<<endl;
     //----销毁资源-------------
     event_del(&listen_ev);
     event_base_free(dispatcher_thread.base);
     cout<<"main() finished"<<endl;

}
