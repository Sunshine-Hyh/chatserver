#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &, json &, Timestamp time)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数(懒汉模式)
    static ChatService *getInstance();

    // 获取消息对应的处理函数
    MsgHandler getHandler(int msgid);

    // 处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理退出登陆业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出业务
    void clientCloseException(const TcpConnectionPtr &conn);

    // 处理服务端异常退出业务
    void reset();

    // 处理点对点聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 从redis消息队列中获取订阅的通道里的消息
    void handleRedisSubscribeMessage(int userid, string msg);

private:
    ChatService();

    // 存储消息id-msgid及其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 数据操作类对象
    UserModel _userModel; // user列表

    OfflineMsgModel _offlineMsgModel; // 离线消息列表

    FriendModel _friendModel; // friend列表

    GroupModel _groupModel; // allgroup表 + groupuser表

    // 存储在线用户id及其对应的连接信息
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 保证_userConnMap线程安全:互斥锁
    mutex _connMutex;

    // redis的操作对象
    Redis _redis;
};

#endif