#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

/*
    redis作为集群聊天服务器通信的基于发布-订阅消息队列时，会遇到两个问题，参考博客详解
    https://blog.csdn.net/QIANGWEIYUAN/article/details/97895611?spm=1001.2014.3001.5501
*/
class Redis
{
public:
    Redis();
    ~Redis();

    // 连接Redis服务器
    bool connect();

    // 向Redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向Redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 向Redis指定的通道unsubscribe取消订阅
    bool unsubscribe(int channel);

    // 独立线程中接收订阅通道的消息
    void observer_channel_message();

    // 初始化业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    // 两个上下文对象：因为subcribe会阻塞，导致publish无法进行。故这两个命令必须放在两个上下文里
    // hiredis同步上下文对象，负责publish消息
    redisContext *publish_context_;

    // hiredis同步上下文对象，负责subscribe消息
    redisContext *subscribe_context_;

    // redis_handler是函数对象：回调操作，收到消息给service上报
    function<void(int, string)> _notify_message_handler;
};

#endif