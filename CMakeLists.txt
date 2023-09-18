cmake_minimum_required(VERSION 3.24)
project(pthread_practice C)

set(CMAKE_C_STANDARD 11)

# 创建服务器可执行文件
add_executable(server server.c socket.h socket.c)

# 创建客户端可执行文件
add_executable(client client.c socket.h socket.c)

