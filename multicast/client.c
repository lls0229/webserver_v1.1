/* 
    单播地址标识单个IP 接口，广播地址标识某个子网的所有IP
    接口，多播地址标识一组IP接口。单播和广播是寻址方案的两个
    极端（要么单个要么全部），多播则意在两者之间提供一种折中
    方案。多播数据报只应该由对它感兴趣的接口接收，也就是说运
    行相应多播会适应应用系统的主机上的接口接收。另外，广播一
    般局限于局域网内使用，而多播则既可以用于局域网，也可以跨
    广域网使用

    int setsockopt
    # server set
    - level IPPROTO_IP
    - optname IP_MULTICAST_IF
    - optval struct in_addr

    # client set
    - level IPPROTO_IP
    - optname IP_ADD_MEMBERSHIP
    - optval struct mreqn

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
    //int op = 1;
    //setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
    //    &op, sizeof(op)); 
    
    /* client bind the IP & port of host */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1){
        perror("bind");
        exit(-1);
    }

    struct ip_mreq op;
    inet_pton(AF_INET, "239.0.0.10", &op.imr_multiaddr.s_addr);
    op.imr_interface.s_addr = INADDR_ANY;
    /* join multicast */
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
        &op, sizeof(op));


    /* communicate */    
    while(1){
        char buf[129];
        /* recv msg */
        int num = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
        printf("server say : %s\n",buf);
    }
    close(fd);

    return 0;
}