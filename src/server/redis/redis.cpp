#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr)
{
}

Redis::~Redis()
{
    if (publish_context_ != nullptr)
    {
        redisFree(publish_context_);
    }

    if (subscribe_context_ != nullptr)
    {
        redisFree(subscribe_context_);
    }
}

// 连接Redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    publish_context_ = redisConnect("127.0.0.1", 6379); // ps -ef | grep redis显示：127.0.0.1:6379 redis默认工作在本地主机的6379端口上。
    if (publish_context_ == nullptr)
    {
        cerr << "publish connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if (subscribe_context_ == nullptr)
    {
        cerr << "subscribe connect redis failed!" << endl;
        return false;
    }

    // 独立线程中(subscribe命令是阻塞的)，监听被订阅通道上的事件，有消息就给业务层上报接收到的订阅通道的消息
    thread t([&]()
             { observer_channel_message(); });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

// 向Redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    // 相当于publish 键 值
    // redis 127.0.0.1:6379> PUBLISH runoobChat "Redis PUBLISH test"
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }

    // 释放资源
    freeReplyObject(reply);
    return true;
}

// 向Redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // publish：redisCommand 会先调用RedisAppendCommand把命令缓存到context中，然后调用redisBufferWrite发送给redis，最后redis会给出一个reply(publish会立即返回不会阻塞)
    // subscribe：redis执行subscribe是阻塞，不会响应，不会给我们一个reply。但是这里我们实现subscribe只做订阅通道，不会阻塞等待接受消息，否则会和notify线程抢占响应资源
    // 通道消息的接收专门在observer_channel_message()中独立线程中进行
    // redis 127.0.0.1:6379> SUBSCRIBE runoobChat
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "SUBSCRIBE %d", channel))
    {
        cerr << "subscibe command failed" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            cerr << "subscribe command failed" << endl;
            return false;
        }
    }
    return true;
}

// 向Redis指定的通道unsubscribe取消订阅
bool Redis::unsubscribe(int channel)
{
    // redisCommand 会先把命令缓存到context中，然后调用RedisAppendCommand发送给redis
    // redis执行subscribe是阻塞，不会响应，不会给我们一个reply
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "UNSUBSCRIBE %d", channel))
    {
        cerr << "subscibe command failed" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            cerr << "subscribe command failed" << endl;
            return false;
        }
    }
    return true;
}

// 独立线程中接收订阅通道的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    // 独立线程以循环阻塞的方式等待上下文有消息发布
    while (REDIS_OK == redisGetReply(subscribe_context_, (void **)&reply))
    {
        // 首先是服务器获取在redis中订阅的通道里的消息
        // reply里面是返回的数据有三个，0.message   1.通道号    2.消息
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报发生的消息：这里才是服务器将消息发送给指定的客户端
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }
    cerr << "----------------------- oberver_channel_message quit--------------------------" << endl;
}

// 初始化业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}