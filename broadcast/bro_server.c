/* broadcast */
/* 
   向子网中多台计算机发送消息，并且子网中所有计算机
   都可以接收到发送方发送的消息，每个广播消息都包含
   一个特殊的IP，这个IP中子网内主机标志部分的二进制
   全部为1
   a. 只能在局域网中使用
   b. 客户端需要绑定服务器广播使用的端口，才可以接收到消息
   int setsockopt(int sockfd, int level, int optname,
                const void* opyval, socklen_t optlen);
    - sockfd 文件描述符
    - level SOL_SOCKET
    - optname SQ_BROADCAT
    - optval int类型的值，为1表示允许广播
    - optlen optval的大小
*/
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>

int main(){
    int fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if(fd == -1){
        perror("socket");
        exit(-1);
    }

    /* set broadcast */
    int op = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
        &op, sizeof(op));
    
    /* create a addr of broadcast */
    struct sockaddr_in cliaddr;
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(9999);
    inet_pton(AF_INET, "192.168.1.255", &cliaddr.sin_addr.s_addr);

    /* communicate */    
    int num = 0;
    while(1){
        char sendBuf[129];
        sprintf(sendBuf, "hello, client...%d\n", num++);

        /* send msg */
        sendto(fd, sendBuf,strlen(sendBuf), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        printf("msg: %s\n", sendBuf);
        sleep(1);

    }
    close(fd);

    return 0;
}