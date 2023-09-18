// client.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include "socket.h"

int main()
{
    // 1. 创建通信的套接字
    int fd = createSocket();

    // 2. 连接服务器
   int ret = connectToHost(fd, "172.16.45.144", 8000);


    // 3. 和服务器端通信
    int fd_re = open("/Users/wangxinnan/CLionProjects/pthread_practice/english.txt", O_RDONLY);
    int length = 0;
    char tmp[1000];
    // 这个read是读取文件的，fd_com是读取文件的描述符
    while((length= read(fd_re, tmp, rand()%1000)) > 0){
        sendMsg(fd, tmp, length);
        memset(tmp, 0, sizeof (tmp));
        usleep(30);
    }
    sleep(10);
    closeSocket(fd);

    return 0;
}
