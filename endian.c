// little-endian : num's high-bit stored at 
//      mem's high-addr, low-bit stored at mem's 
//      mem's low-addr. 
// big-endian : nums's low-bit stored at mem's
//      high-addr, high-bit stored at mem's
//      low-addr.
// almost ps use little-endian

#include<stdio.h>

int main(){

    union{
        short value;  // 2 Byte
        char byte[sizeof(short)];  // char[2]
    }test;

    test.value = 0x0102;
    if(test.byte[0] == 1
        && test.byte[1] == 2){
        printf("big-endian\n");
    }
    else if(test.byte[0] == 2
            && test.byte[1] == 1){
        printf("little-endian\n");
    }

    return 0;
}