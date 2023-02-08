/* select() */

#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<sys/time.h>
#include<vector>
#include<errno.h>

/* 自定义代表无效fd 的值 */
#define INVALID_FD -1
int main(int argc, char const *argv[])
{
    /* create a socket */
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1){
        std::cout << "create listen socket error." << std::endl;
        return -1;
    }

    /* init server addr */
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(3000);
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* bind */
    if(bind(lfd, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) == -1){
        std::cout << "bind listen socker error." <<std::endl;
        close(lfd);
        return -1;
    }

    /* listen */
    if(listen(lfd, SOMAXCONN) == -1){
        std::cout << "listen error." <<std::endl;
        close(lfd);
        return -1;
    }

    /* socket-vec */
    std::vector<int> clifds;
    int maxfd = lfd;

    while(1){
        fd_set readset;
        FD_ZERO(&readset);

        /* add lfd to readable event be detected */
        FD_SET(lfd, &readset);

        /* add clifd to readable event be detected */
        int clifds_len = clifds.size();
        for(int i = 0; i < clifds_len; i++){
            if(clifds[i] != INVALID_FD){
                FD_SET(clifds[i], &readset);
            }
        }

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        /* only readable events are detected,
            not writable and exception events */
        int ret = select(maxfd +1, &readset, NULL, NULL, &tv);
        if(ret == -1){
            /* error, exit */
            if(errno != EINTR){
                break;
            }
        }else if(ret == 0 ){
            /* select timeout, nexttime continue */
            continue;
        }else {
            /* detecte a socket get event */
            if(FD_ISSET(lfd, &readset)){
                struct sockaddr_in cliaddr;
                socklen_t cliaddr_len = sizeof(cliaddr);
                /* accept client connection */
                int clifd = accept(lfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
                if(clifd == -1){
                    /* acceot error, exit */
                    break;
                }

                /* only accept, not calling recv to recv data */
                std::cout << "accept a client connecttion, fd: " << clifd << std::endl;
                clifds.push_back(clifd);
                /* record the newest maxfd to be used as the 
                    first arg of select in next loop */
                if(clifd > maxfd){
                    maxfd = clifd;
                }
            }
            else{
                /* assume data-len sent from peer donot exceed 63 */
                char recvbuf[64];
                int clifds_len = clifds.size();
                for(int i = 0; i < clifds_len; i++){
                    if(clifds[i] != -1 && FD_ISSET(clifds[i], &readset)){
                        memset(recvbuf, 0, sizeof(recvbuf));
                        /* not listen socket, just recv data */
                        int len = recv(clifds[i], recvbuf, 64, 0);
                        if(len <= 0 && errno != EINTR){
                            /* recv error */
                            std::cout << "recb data error, clifd: " <<clifds[i] << std::endl;
                            close(clifds[i]);
                            /* not delete index, set index-pos to -1 */
                            clifds[i] = INVALID_FD;
                            continue;
                        }

                        std::cout << "clifd: "<< clifds[i] << ", recv data: "<< recvbuf << std::endl;
                    }
                }
            }
        
        }
        


    }


    /* close all client-socket */
    int clifds_len = clifds.size();
    for(int i = 0; i < clifds_len; i++){
        if(clifds[i] != INVALID_FD){
            close(clifds[i]);
        }
    }

    /* close listen socket */
    close(lfd);




    return 0;
}





