/* 基于epoll的回声服务器(默认LT) */
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>

#define MAX_EVENTS 10
#define SERVER_PORT 9999
#define SERVER_ADDR INADDR_ANY
#define EPOLL_SIZE 0

void setnoblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int res = fcntl(fd, F_SETFL, flags);
    if(res < 0){
        perror("fcrnl");
        exit(EXIT_FAILURE);
    }
}

int main(){

    struct epoll_event ev, events[MAX_EVENTS];
    int listen_sock, conn_sock, nfds;

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    saddr.sin_addr.s_addr = htonl(SERVER_ADDR);

    int ret = bind(listen_sock, (struct sockaddr *)&saddr, sizeof(saddr));
    if(ret == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    ret = listen(listen_sock, 128);
    if(ret == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int efd = epoll_create1(EPOLL_SIZE);
    if(efd == -1){
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }    

    //events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);


    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, listen_sock, &ev) == -1){
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    for(;;){
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        char recvbuf[128];
        
        nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
        if(nfds == -1){
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        int i;
        for(i = 0; i < nfds; i++){
            if(events[i].data.fd == listen_sock){
                conn_sock = accept(listen_sock,
                                (struct sockaddr *)&addr, &addrlen);
                if(conn_sock == -1){
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                setnoblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if(epoll_ctl(efd, EPOLL_CTL_ADD, conn_sock, &ev) == -1){
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
                printf("connected client: %d\n",conn_sock);
            }else{
                // do_use_fd(events[i].data.fd);

                int len = read(events[i].data.fd, recvbuf, sizeof(recvbuf));
                if(len == 0){
                    epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                    close(events[i].data.fd);
                    printf("closed client: %d\n", events[i].data.fd);
                    break;
                }
                else if(len < 0){
                    if(errno == EAGAIN){
                        break;
                    }
                }
                else {
                    write(events[i].data.fd, recvbuf, len);
                }
            }
        }
    }
    close(listen_sock);
    close(efd);
    return 0;
}
