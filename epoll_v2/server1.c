#include "server1.h"

static void* socketPthreadRun(void* arg)
{
    socket_thd_struct* pa_sock_st=(socket_thd_struct*)arg;
    int active_counts=0;
    struct epoll_event ev;
    struct epoll_event events[5];
    int ret=0;

    while(1)
    {
        //等待读写事件的到来
        active_counts=epoll_wait(pa_sock_st->epoll_fd,events,5,-1);
        fprintf(stdout,"active count:%d\n",active_counts);

        int index=0;
        for(index=0;index<active_counts;index++)
        {
            if(events[index].events&EPOLLERR) //发生异常错误
            {
                fprintf(stderr,"error happened:errno(%d)-%s\n",errno,strerror(errno));
                epoll_ctl(pa_sock_st->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                close(events[index].data.fd);

                pthread_mutex_lock(&(pa_sock_st->thd_mutex));
                pa_sock_st->active_conection_cnt--;
                pthread_mutex_unlock(&(pa_sock_st->thd_mutex));
            }
            else if(events[index].events&EPOLLRDHUP) //对端异常关闭连接
            {
                fprintf(stdout,"client close this socket\n");
                epoll_ctl(pa_sock_st->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                close(events[index].data.fd);

                pthread_mutex_lock(&(pa_sock_st->thd_mutex));
                pa_sock_st->active_conection_cnt--;
                pthread_mutex_unlock(&(pa_sock_st->thd_mutex));
            }
            else if(events[index].events&EPOLLIN) //读事件到来，进行消息处理
            {
                ret=pa_sock_st->data_func(events[index].data.fd,NULL);
                if(ret==-1)
                {
                    fprintf(stderr,"client socket exception happened\n");
                    epoll_ctl(pa_sock_st->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                    close(events[index].data.fd);

                    pthread_mutex_lock(&(pa_sock_st->thd_mutex));
                    pa_sock_st->active_conection_cnt--;
                    pthread_mutex_unlock(&(pa_sock_st->thd_mutex));
                }
                if(ret==0)
                {
                    fprintf(stdout,"client close this socket\n");
                    epoll_ctl(pa_sock_st->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                    close(events[index].data.fd);

                    pthread_mutex_lock(&(pa_sock_st->thd_mutex));
                    pa_sock_st->active_conection_cnt--;
                    pthread_mutex_unlock(&(pa_sock_st->thd_mutex));
                }
                else if(ret==1)
                {
                    
                }
            }
        }
    }

    pthread_exit(NULL);
}

server_struct* initServerStruct(unsigned short param_port,unsigned int param_thd_count,MSG_HANDLE param_handle)
{
    server_struct* serv_st=(server_struct*)malloc(sizeof(server_struct));
    serv_st->ip_port=param_port;
    serv_st->data_func=param_handle;
    serv_st->epoll_fd=epoll_create(256);
    serv_st->socket_pthread_count=param_thd_count;
    serv_st->socket_thd=NULL;

    if(serv_st->socket_pthread_count>0)
    {
        fprintf(stdout,"create client socket sub thread\n");
        serv_st->socket_thd=(socket_thd_struct*)malloc(sizeof(socket_thd_struct)*serv_st->socket_pthread_count);

        int index=0;
        for(index=0;index<serv_st->socket_pthread_count;index++)
        {
            serv_st->socket_thd[index].epoll_fd=epoll_create(256);
            serv_st->socket_thd[index].data_func=param_handle;
            serv_st->socket_thd[index].active_conection_cnt=0;
            serv_st->socket_thd[index].thd_fd=0;
            //创建子线程
            pthread_create(&(serv_st->socket_thd[index].thd_fd),NULL,socketPthreadRun,(void*)&(serv_st->socket_thd[index]));
            //初始化线程互斥锁
            pthread_mutex_init(&(serv_st->socket_thd[index].thd_mutex),NULL);
        }
    }

    return serv_st;
}

int serverRun(server_struct *param_server)
{
    int ret=RESULT_OK;
    int server_socket=0;
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    struct epoll_event ev;
    struct epoll_event events[5];
    int active_count=0;
    int index=0;
    int new_socket=0;
    struct sockaddr_in client_info;
    socklen_t client_info_len=0;

    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htons(INADDR_ANY);
    server_addr.sin_port=htons(param_server->ip_port);

    server_socket=socket(PF_INET,SOCK_STREAM,0);
    if(server_socket<0)
    {
        fprintf(stderr,"create socket error:errno(%d)-%s\n",errno,strerror(errno));
        return RESULT_ERROR;
    }
    fprintf(stdout,"create server socket ssuccessful\n");

    param_server->epoll_fd=epoll_create(256);

    ev.data.fd=server_socket;
    ev.events=EPOLLIN|EPOLLET;
    if(epoll_ctl(param_server->epoll_fd,EPOLL_CTL_ADD,server_socket,&ev)!=0)
    {
        fprintf(stderr,"server socket add to epoll error:errno(%d)-%s\n",errno,strerror(errno));
        return RESULT_ERROR;
    }
    fprintf(stdout,"server socket add to epoll successful\n");

    if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))!=0)
    {
        fprintf(stderr,"server bind failed:errno(%d)-%s\n",errno,strerror(errno));
        return RESULT_ERROR;
    }
    fprintf(stdout,"server socket bind successful\n");

    if(listen(server_socket,param_server->ip_port)!=0)
    {
        fprintf(stderr,"server listen failed:errno(%d)-%s\n",errno,strerror(errno));
        return RESULT_ERROR;
    }
    fprintf(stdout,"server socket listen successful\n");

    while(1)
    {
        active_count=epoll_wait(param_server->epoll_fd,events,5,-1);
        fprintf(stdout,"active count:%d\n",active_count);

        for(index=0;index<active_count;index++)
        {
            if(events[index].data.fd==server_socket) //新连接过来
            {
                fprintf(stdout,"new socket comming\n");
                client_info_len=sizeof(client_info);
                new_socket=accept(server_socket,(struct sockaddr*)&client_info,&client_info_len);
                if(new_socket<0)
                {
                    fprintf(stderr,"server accept failed:errno(%d)-%s\n",errno,strerror(errno));
                    continue;
                }

                fprintf(stdout,"new socket:%d.%d.%d.%d:%d-->connected\n",((unsigned char*)&(client_info.sin_addr))[0],((unsigned char*)&(client_info.sin_addr))[1],((unsigned char*)&(client_info.sin_addr))[2],((unsigned char*)&(client_info.sin_addr))[3],client_info.sin_port);

                ev.data.fd=new_socket;
                ev.events=EPOLLIN|EPOLLERR|EPOLLRDHUP;

                if(param_server->socket_pthread_count==0)
                {
                    epoll_ctl(param_server->epoll_fd,EPOLL_CTL_ADD,new_socket,&ev);
                }
                else
                {
                    int tmp_index=0;
                    int mix_cnt_thread_id=0;
                    unsigned int act_cnts=0;
                    for(tmp_index=0;tmp_index<param_server->socket_pthread_count;tmp_index++)
                    {
                        pthread_mutex_lock(&(param_server->socket_thd[tmp_index].thd_mutex));
                        act_cnts=param_server->socket_thd[tmp_index].active_conection_cnt;
                        pthread_mutex_unlock(&(param_server->socket_thd[tmp_index].thd_mutex));
                        if(mix_cnt_thread_id>act_cnts)
                        {
                            mix_cnt_thread_id=tmp_index;
                        }
                    }

                    epoll_ctl(param_server->socket_thd[mix_cnt_thread_id].epoll_fd,EPOLL_CTL_ADD,new_socket,&ev);

                    pthread_mutex_lock(&(param_server->socket_thd[mix_cnt_thread_id].thd_mutex));
                    param_server->socket_thd[mix_cnt_thread_id].active_conection_cnt++;
                    pthread_mutex_unlock(&(param_server->socket_thd[mix_cnt_thread_id].thd_mutex));
                }

                fprintf(stdout,"add new client socket to epoll\n");
            }
            else if(events[index].events&EPOLLERR || events[index].events&EPOLLRDHUP) //对端关闭连接
            {
                fprintf(stdout,"client close this socket\n");
                epoll_ctl(param_server->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                close(events[index].data.fd);
            }
            else if(events[index].events&EPOLLIN) //读事件到来，进行消息处理
            {
                fprintf(stdout,"begin recv client data\n");
                ret=param_server->data_func(events[index].data.fd,NULL);
                if(ret==-1)
                {
                    fprintf(stderr,"client socket exception happened\n");
                    epoll_ctl(param_server->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                    close(events[index].data.fd);
                }
                if(ret==0)
                {
                    fprintf(stdout,"client close this socket\n");
                    epoll_ctl(param_server->epoll_fd,EPOLL_CTL_DEL,events[index].data.fd,NULL);
                    close(events[index].data.fd);
                }
                else if(ret==1)
                {
                    
                }
            }
        }
    }

    close(server_socket);
    return RESULT_OK;
}
