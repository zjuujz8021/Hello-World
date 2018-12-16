#include <stdio.h>

int main()
{
    long temp= 0x12345678aabbccdd;

    printf("[*] pid is %d temp address %p\n", getpid(), &temp);
    while(1);
    return 0;
}
