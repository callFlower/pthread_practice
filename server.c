// server.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "socket.h"

// 每个客户端都有自己的ip和端口，对应的文件描述符也不同
// 所以我们需要创建出n个--也就是要创建一个数组
struct SockInfo{
    struct sockaddr_in addr;
    int fd;
};
struct SockInfo infos[512];

void* working(void* arg);

int main() {
    // 1. 创建监听的套接字
    // IPV4, 流式协议, TCP -- 三个参数
    int fd = createSocket();

    // 2. 将socket()返回值和本地的IP端口绑定到一起 + 设置监听
    int ret = setListen(fd, 8000);

    // 初始化结构体数组
    int max = sizeof(infos) / sizeof(infos[0]);
    for (int i = 0; i < max; i++) {
        // 把结构体每个元素初始化为0
        bzero(&infos[i], sizeof(infos[i]));
        infos[i].fd = -1;
    }

    // 4. 阻塞等待并接受客户端连接
    while (1) {
        struct SockInfo *pinfo;
        for (int i = 0; i < max; i++) {
            if (infos[i].fd == -1) {
                pinfo = &infos[i];
                break;
            }
        }

        pinfo->fd = acceptConn(fd,&pinfo->addr);

        // 创建子线程
        pthread_t tid;
        pthread_create(&tid, NULL, working, pinfo);
        // 主线程回收join，但这个是个阻塞函数，也就阻塞住主线程，accept就不能用了。所以我们需要调用分离函数
        // 指定的子线程就可以和主线程分离
        pthread_detach(tid);
    }
    return 0;
}

void* working(void* arg)
{
    struct SockInfo* pinfo = (struct SockInfo*)arg;
    // 打印客户端的地址信息
    char ip[32] = {0};
    printf("客户端的IP地址: %s, 端口: %d\n",
            // 要转换成小端的在输出
           inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, ip, sizeof(ip)),
            // 把端口取出来
           ntohs(pinfo->addr.sin_port));
    // 5. 和客户端通信
    while(1)
    {
        // 接收数据
        // 我们这里设置接收端可以接受4k个数据，而发送端一次只能发送100个字节
        char* buf;
        int len = recvMsg(pinfo->fd, &buf);
        // 说明接收了一定量的数据
        if(len > 0)
        {
            // 将客户端发送过来的打印出来
            printf("客户端say: %s\n", buf);
            // 用完后记得防止内存泄漏
            free(buf);
        }
        else
        {
            break;
        }
        // 服务器端接收数据时间间隔是1s
        // 但客户端发送时间间隔是300us
        sleep(1);
    }
    // 还需要进行回收--也就是将数组里面那个描述符改为可用的-- -1
    pinfo->fd = -1;
    return NULL;
}

