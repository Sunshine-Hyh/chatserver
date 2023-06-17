#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include <iostream>
#include <string>

using namespace std;
using namespace placeholders;
using json = nlohmann::json; // 这里将作用域简短化成json

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop* loop,      // 事件循环
            const InetAddress& listenAddr,   // IP + Port
            const string& nameArg)           // 服务器的名称
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
        /*
            1. muduo的源码中，TcpServer类的成员函数void setConnectionCallback(const ConnectionCallback& cb) cb即为我们自己要实现的回调函数
               ConnectionCallback是通过typedef起的别名：typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
            2. 因此我们写的回调函数中需要的参数为：
               成员函数中隐含传入一个this + const TcpConnectionPtr&，通过bind()进行绑定，返回函数对象
               _1作为占位符代表onConnection中传入的参数：const TcpConnectionPtr&
        */

        // 给服务器注册用户读写事件发生时的回调
        _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量：4 = 1(I/O线程) + 3(worker线程)
        _server.setThreadNum(4);
}
    
// 启动服务：开启事件循环
void ChatServer::start()
{
    _server.start();
}
    

// 专门处理用户的连接创建和断开的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    // 连接失败：相当于客户端断开连接，用户下线
    if (!conn -> connected())
    {
        // 异常断开业务处理函数
        ChatService::getInstance() -> clientCloseException(conn);
        conn -> shutdown();  // 关闭socketfd
    }
}

// 专门处理用户的读写事件
void ChatServer::onMessage(const TcpConnectionPtr& conn,  // 连接
                        Buffer* buffer,       // 缓冲区
                        Timestamp time)       // 接收到数据的时间信息
{
    // 1. 从缓冲区中读取客户端发送过来的数据
    string buf = buffer -> retrieveAllAsString();
    /*
        buffer是muduo中的一个类，通过retrieveAllAsString
            将缓冲区的数据存入字符串buf中，并且更新readerIndex_ = writerIndex_ = kCheapPrepend，
            readerIndex_是从缓冲区中读取数据的起始位置，writerIndex_是向缓冲区中写入数据的起始位置  
    */

   // 2. 数据的反序列化
   json js = json::parse(buf);  //  将字符串buf  →反序列化  json数据对象js从而提取出我们所需要的信息   

   // 3. 网络模块与业务模块的解耦：定义一个业务处理类，其成员函数用来处理不同的业务。
   // 根据js["msgid"]  调用相对应的业务处理函数handler，并将conn、js、time传递给handler作为其参数
   auto msgHandler = ChatService::getInstance() -> getHandler(js["msgid"].get<int>());  // js[msgid]还是一个整型，通过get()将其转换为int型
   msgHandler(conn, js, time);
}