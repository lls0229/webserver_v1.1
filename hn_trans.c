#include<stdio.h>
#include<arpa/inet.h>
typedef unsigned short u_sh;
typedef unsigned char u_ch;
int main(){

    // htons convert port
    u_sh a = 0x0102;
    u_sh b = htons(a);
    printf("a = %x, b = %x\n", a, b);
printf("__________________________\n");
    // htonl convert IP
    u_ch buf[4] = {192, 168,1, 100};
    int num = *(int *)buf;
    int sum = htonl(num);
    u_ch *p = (u_ch *)&sum;
    printf("%d %d %d %d\n", *p, *(p+1), *(p+2), *(p+3));
printf("__________________________\n");

    // ntohl convert prot
    u_ch buf1[4] = {1,1,168,192};
    int num1 = *(int *)buf1;
    int sum1 = ntohl(num1);
    u_ch *p1 = (u_ch*)&sum1;
    printf("%d %d %d %d\n", *p1, *(p1+1), *(p1+2), *(p1+3));
printf("__________________________\n");

    // ntohs convert IP
    u_sh c = 0x0102;
    u_sh d = ntohs(c);
    printf("c = %x, d = %x\n",c, d);

    return 0;
}