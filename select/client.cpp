/* 验证调用select 后必须重设fd_set */

#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT    3000

int main(int argc, char const *argv[])
{
    /* create a socket */
    int clifd = socket(AF_INET, SOCK_STREAM, 0);
    if(clifd == -1){
        std::cout << "create client socket error." << std::endl;
        return -1;
    }

    /* connect */
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    saddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    if(connect(clifd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1){
        std::cout << "connect socket errror." << std::endl;
        return -1;
    }

    fd_set readset;
    FD_ZERO(&readset);

    /* add clifd to readable events be detected */
    FD_SET(clifd, &readset);
    timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int ret;
    int count = 0;
    fd_set backup_readset;
    memcpy(&backup_readset, &readset, sizeof(fd_set));
    while(1){
        if(memcmp(&readset, &backup_readset, sizeof(fd_set)) == 0){
            std::cout << "equal" << std::endl;
        }else{
            std::cout << "not equal" << std::endl;
        }

        /* only readable events are detected,
            not writable and exception events */
        ret = select(clifd + 1, &readset, NULL, NULL, &tv);
        std::cout << "tm.tv_sec: " << tv.tv_sec << ", tv.tv_usec: " << tv.tv_usec << std::endl;
        if(ret == -1){
            /* except signal is interrupted,others are error */
            if(errno != EINTR)
                break;
        }else if(ret == 0){
            /* select timeout */
            std::cout << "no event in specific time interval, count: " << count << std::endl;
            ++count;
            sleep(1);
            continue;
        }else{
            if(FD_ISSET(clifd, &readset)){
                /* readable events be detected */
                char recvbuf[32];
                memset(recvbuf, 0, sizeof(recvbuf));
                /* assume data-len sent from peer donot exceed 63 */
                int n = recv(clifd, recvbuf, 32, 0);
                if(n < 0){
                    /* except signal is interrupted,others are error */
                    if(errno != EINTR)
                        break;
                }else if(n == 0){
                    /* the peer close connect */
                    break;
                }else{
                    std::cout << "recv data: " << recvbuf << std::endl;
                }
            }
            else{
                std::cout << "other socket event." <<std::endl;
            }
        }
    }

    /* close socket */
    close(clifd);
    return 0;
}
