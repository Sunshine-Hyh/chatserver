#ifndef CHATSERVER_H
#define CHATSERVER_H


/*
    muduo网络库给用户提供了两个主要的类：
        TcpServer：用于编写服务器程序
        TcpClient：用于编写客户端程序
    使用muduo好处：能够将网络I/O代码与业务代码（用户的连接和断开）区分开
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;


/*
基于muduo网络库开发服务器程序的流程：
    1. 组合TcpServer对象
    2. 创建EventLoop事件循环对象指针
    3. 明确TcpServer构造函数需要什么参数，通过Chatserver的构造函数向其提供相应的参数
    4. 在当前服务器类的构造函数中，注册 处理连接的回调函数 + 处理读写事件的回调函数
    5. 设置合适的服务器线程数量 n，muduo库会自己分配I/O线程（1）和worker线程（n - 1）
*/

// 聊天服务器的主类
class  ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop* loop,                 // 事件循环
               const InetAddress& listenAddr,   // IP + Port
               const string& nameArg);          // 服务器的名称 
    
    // 启动服务：开启事件循环
    void start();
    
private:
    // 专门处理用户的连接创建和断开的回调函数
    void onConnection(const TcpConnectionPtr& conn);

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,  // 连接
                            Buffer* buffer,       // 缓冲区
                            Timestamp time);       // 接收到数据的时间信息

    TcpServer _server;  // 组合的muduo库，实现服务器功能的类对象
    EventLoop* _loop;   // 指向事件循环对象的指针

};


#endif