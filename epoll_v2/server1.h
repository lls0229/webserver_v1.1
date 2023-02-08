#ifndef EPOLL_C_SERVER_H
#define EPOLL_C_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define RESULT_OK 0
#define RESULT_ERROR -1

/**************************************************************************
* Function    : *MSG_HANDLE
* Input       : socket_fd --> socket文件描述符
*             : arg --> void* 参数
* Output      :
* Return      : 1 处理成功，继续等待下次消息;0 处理完毕完毕该连接;-1 异常错误发生
* Description : 消息处理函数指针
****************************************************************************/
typedef int (*MSG_HANDLE)(int socket_fd,void* arg) ;  

typedef struct
{
    int epoll_fd;
    pthread_t thd_fd;
    
    //消息处理函数，各个线程会调用该函数进行消息处理
    MSG_HANDLE data_func;
	
    //一个线程里面的有效socket数量
    unsigned int active_conection_cnt; 
    //线程互斥锁，用于实时更新有效socket数量
    pthread_mutex_t thd_mutex;  
}socket_thd_struct;   //表示处理socket的子线程

typedef struct
{
    int epoll_fd;
    unsigned short ip_port;
    
    //消息处理函数，当只有一个线程时，会调用该函数进行消息处理
    MSG_HANDLE data_func;

    //子线程数量，可以为0，为0表示server与socket处理处于同一个线程
    unsigned int socket_pthread_count; 
    //子线程结构体指针
    socket_thd_struct* socket_thd;   

}server_struct;  //一个网络服务结构体

/**************************************************************************
* Function    : initServerStruct
* Input       : param_port --> 服务端口号
*             : param_thd_count --> 子线程数量，用于处理连接的client
*             : param_handle --> socket数据处理函数指针
* Output      :
* Return      : 初始化好的server结构体
* Description : 初始化server结构体
****************************************************************************/
server_struct* initServerStruct(unsigned short param_port,unsigned int param_thd_count,MSG_HANDLE param_handle);

/**************************************************************************
* Function    : serverRun
* Input       : param_server --> server服务结构体参数
* Output      :
* Return      :RESULT_OK(0)：执行成功；RESULT_ERROR(-1)：执行失败
* Description : 运行网络服务，监听服务端口
****************************************************************************/
int serverRun(server_struct *param_server);

#endif //EPOLL_C_SERVER_H
