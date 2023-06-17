/*
    muduo网络库给用户提供了两个主要的类：
        TcpServer：用于编写服务器程序
        TcpClient：用于编写客户端程序
    使用muduo好处：能够将网络I/O代码与业务代码（用户的连接和断开）区分开
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*
基于muduo网络库开发服务器程序的流程：
    1. 组合TcpServer对象
    2. 创建EventLoop事件循环对象指针
    3. 明确TcpServer构造函数需要什么参数，通过Chatserver的构造函数向其提供相应的参数
    4. 在当前服务器类的构造函数中，注册 处理连接的回调函数 + 处理读写事件的回调函数
    5. 设置合适的服务器线程数量 n，muduo库会自己分配I/O线程（1）和worker线程（n - 1）
*/

class  ChatServer
{
public:
    ChatServer(EventLoop* loop,                 // 事件循环
               const InetAddress& listenAddr,   // IP + Port
               const string& nameArg)           // 服务器的名称
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
            // 给服务器注册用户连接的创建和断开回调
            _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
            /*
                muduo的源码中，TcpServer类的成员函数void setConnectionCallback(const ConnectionCallback& cb) cb即为我们自己要实现的回调函数
                ConnectionCallback是通过typedef起的别名：typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
                因此我们写的回调函数中需要的参数为：const TcpConnectionPtr&
                但是由于我们写的回调函数为成员函数，可以修改访问成员变量，因此还有传入一个this，通过bind()进行绑定
                _1作为占位符代表onConnection中传入的参数
                相当于：this, _1是要传递给onConnection，再将onConnection传递给setConnectionCallback
            */

            // 给服务器注册用户读写事件发生时的回调
            _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

            // 设置服务器端的线程数量：4 = 1(I/O线程) + 3(worker线程)
            _server.setThreadNum(4);
    }
    
    // 开启事件循环
    void start()
    {
        _server.start();
    }
    
private:
    // 专门处理用户的连接创建和断开的回调函数
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn -> connected())
        {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " start:online" << endl; // 输出客户端的IP + 端口号  本地的IP + 端口号
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " start:offline" << endl;
            conn -> shutdown(); // close(fd)，回收服务器端的fd

            // _loop->quit();  // 相当于退出epoll不再给客户端提供服务
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,  // 连接
                            Buffer* buffer,       // 缓冲区
                            Timestamp time)       // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString(); // 将缓冲区的数据返回给string
        cout << "recv data: " << buf << " time: " << time.toString() << endl;
        conn -> send(buf); // 服务器向对端发送数据 buf
    }
    TcpServer _server;
    EventLoop* _loop;

};

int main()
{
    EventLoop loop;
    InetAddress addr("c", 6000);

    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();   // epoll_wait以阻塞方式等待新用户的连接 + 已连接用户的读写事件

    return 0;
}