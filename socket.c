#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
int writen(int fd, const char* msg, int size);
int readn(int fd, char* buf, int size);

// 创建监听套接字
int createSocket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket");
        return -1;
    }
    printf("套接字创建成功, fd=%d\n", fd);
    return fd;
}

// 绑定本地的IP和端口 + 设置监听
int setListen(int lfd, unsigned short port)
{
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;  // 0 = 0.0.0.0 绑定本地任意一个ip
    int ret = bind(lfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("bind");
        return -1;
    }
    printf("套接字绑定成功, ip: %s, port: %d\n",
           inet_ntoa(saddr.sin_addr), port);
    ret = listen(lfd, 128);
    if(ret == -1)
    {
        perror("listen");
        return -1;
    }
    printf("设置监听成功...\n");
    return ret;
}

// 阻塞并等待客户端的连接
// 第二个参数是传出参数
int acceptConn(int lfd, struct sockaddr_in *addr)
{
    int cfd = -1;
    if(addr == NULL)
    {
        cfd = accept(lfd, NULL, NULL);
    }
    else
    {
        int addrlen = sizeof(struct sockaddr_in);
        cfd = accept(lfd, (struct sockaddr*)addr, &addrlen);
    }
    if(cfd == -1)
    {
        perror("accept");
        // 也许考虑退出程序或更加优雅地处理错误
        exit(EXIT_FAILURE);
    }
    printf("成功和客户端建立连接...\n");
    return cfd;
}

// 接收数据
/*
函数描述: 接收带数据头的数据包
函数参数:
    - cfd: 通信的文件描述符(套接字)
    - msg: 一级指针的地址，函数内部会给这个指针分配内存，用于存储待接收的数据，这块内存需要使用者释放
函数返回值: 函数调用成功返回接收的字节数, 发送失败返回-1
*/
int recvMsg(int cfd, char** msg)
{
    // 接收数据
    // 1. 读数据头
    int len = 0;
    readn(cfd, (char*)&len, 4);
    len = ntohl(len);
    printf("数据块大小: %d\n", len);

    // 根据读出的长度分配内存，+1 -> 这个字节存储\0
    char *buf = (char*)malloc(len+1);
    int ret = readn(cfd, buf, len);
    if(ret != len)
    {
        printf("接收数据失败了...");
        close(cfd);
        free(buf);
        return -1;
    }
    buf[len] = '\0';
    *msg = buf;

    return ret;
}

// 发送数据
/*
函数描述: 发送带有数据头的数据包
函数参数:
    - cfd: 通信的文件描述符(套接字)
    - msg: 待发送的原始数据
    - len: 待发送的原始数据的总字节数
函数返回值: 函数调用成功返回发送的字节数, 发送失败返回-1
*/
int sendMsg(int cfd, char* msg, int len)
{
    if(msg == NULL || len <= 0 || cfd <=0)
    {
        return -1;
    }
    // 申请内存空间: 数据长度 + 包头4字节(存储数据长度)
    char* data = (char*)malloc(len+4);
    int bigLen = htonl(len);
    memcpy(data, &bigLen, 4);
    memcpy(data+4, msg, len);
    // 发送数据
    int ret = writen(cfd, data, len+4);
    // 释放内存
    free(data);
    return ret;
}


// 连接服务器
int connectToHost(int fd, const char* ip, unsigned short port)
{
    // 2. 连接服务器IP port
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &saddr.sin_addr.s_addr);
    int ret = connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("connect");
        return -1;
    }
    printf("成功和服务器建立连接...\n");
    return ret;
}

// 关闭套接字
int closeSocket(int fd)
{
    int ret = close(fd);
    if(ret == -1)
    {
        perror("close");
    }
    return ret;
}

/*
函数描述: 发送指定的字节数
函数参数:
    - fd: 通信的文件描述符(套接字)
    - msg: 待发送的原始数据
    - size: 待发送的原始数据的总字节数
函数返回值: 函数调用成功返回发送的字节数, 发送失败返回-1
*/
int writen(int fd, const char* msg, int size)
{
    const char* buf = msg;
    int count = size;
    while (count > 0)
    {
        int len = send(fd, buf, count, 0);
        if (len == -1)
        {
            close(fd);
            return -1;
        }
        else if (len == 0)
        {
            continue;
        }
        buf += len;
        count -= len;
    }
    return size;
}

/*
函数描述: 接收指定的字节数
函数参数:
    - fd: 通信的文件描述符(套接字)
    - buf: 存储待接收数据的内存的起始地址
    - size: 指定要接收的字节数
函数返回值: 函数调用成功返回发送的字节数, 发送失败返回-1
*/
int readn(int fd, char* buf, int size)
{
    char* pt = buf;
    int count = size;
    while (count > 0)
    {
        int len = recv(fd, pt, count, 0);
        if (len == -1)
        {
            return -1;
        }
        else if (len == 0)
        {
            // 返回已经接收到的字节数
            return size - count;
        }
        pt += len;
        count -= len;
    }
    return size;
}