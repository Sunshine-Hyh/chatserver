#include <muduo/base/Logging.h>
#include <iostream>
#include <vector>
#include "chatservice.hpp"
#include "public.hpp"

using namespace muduo;
using namespace std;

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});

    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});

    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接Redis服务器
    if (_redis.connect())
    {
        // 通过init_notify_handler来初始化_notify_message_handler ———— 用来将发送给指定通道的信息发送出去
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取单例对象的接口函数
ChatService *ChatService::getInstance()
{
    // C++11保证静态局部对象是线程安全的
    static ChatService chat;
    return &chat;
}

// 获取消息对应的处理函数
MsgHandler ChatService::getHandler(int msgid)
{
    if (_msgHandlerMap.find(msgid) == _msgHandlerMap.end())
    {
        // 找不到相关的回调函数，则返回一个默认的处理器(lambda匿名函数，仅仅用作提示)，该函数记录错误日志
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            // muduo已经封装好了相应的日志处理函数LOG_ERROR
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
        return _msgHandlerMap[msgid];
}

// 处理登陆业务
void ChatService::ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 1. 提取信息：id和password
    int id = js["id"].get<int>();
    string pwd = js["password"];

    // 2. 凭借_userModel进行查询(通过MYSQL的API查询)
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登陆，不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this count is using, please input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，存储用户的连接信息 + 更新用户的状态信息  offline → online + 查询是否有离线消息 + 返回好友列表 + 查询用户的群组信息

            // 1. 存储用户的连接信息同时保证线程安全
            {
                lock_guard<mutex> lock(_connMutex);
                /*
                    lock_guard是类，利用oop思想实现自动加锁和释放锁：lock_guard的构造中lock()，析构时候unlock()
                    因此仅限于该作用域范围加锁解锁
                */
                _userConnMap[id] = conn;
            }

            // 用户登录成功后向redis的消息队列中订阅以用户id为名的通道
            _redis.subscribe(id);

            // 2. 更新用户的状态信息
            user.setState("online");
            if (_userModel.updateState(user))
                LOG_INFO << "updateState successful!!";
            else
                LOG_INFO << "updateState failed!!";
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 3. 查询用户是否有离线消息：如果有就发送过去，并且将offlinemessage表格中的数据删除
            vector<string> vec_offlinemessage = _offlineMsgModel.query(id);
            if (!vec_offlinemessage.empty())
            {
                response["offlinemsg"] = vec_offlinemessage;
                _offlineMsgModel.remove(id);
            }

            // 4. 返回用户的好友列表信息
            vector<User> vec_friendlist = _friendModel.query(id);
            if (!vec_friendlist.empty())
            {
                vector<string> vec_friend;
                for (User &user : vec_friendlist)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec_friend.push_back(js.dump());
                }
                response["friends"] = vec_friend;
            }

            // 5. 查询用户的群组信息：群id、群名、群描述、群成员
            vector<Group> vec_grouplist = _groupModel.queryGroups(id);
            if (!vec_grouplist.empty())
            {
                vector<string> vec_group;
                for (Group &group : vec_grouplist)
                {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();
                    vector<string> vec_user;
                    for (GroupUser &user : group.getUser())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        vec_user.push_back(js.dump());
                    }
                    js["users"] = vec_user;
                    vec_group.push_back(js.dump());
                }
                response["groups"] = vec_group;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败:用户不存在或者用户存在但是密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理退出登陆业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        // 删除对应的连接信息
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于是下线，在redis中取消订阅
    _redis.unsubscribe(userid);

    // 更新用户状态信息：state改为offline
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 在服务器类中，读取到客户端发送过来的字符串已经被反序列化为对象，我们从中提取用到的信息即可
    // 由于是注册，只需要提取name password，id是数据库自增，向数据库中添加记录后就可以获取；state默认是offline
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功，向客户端发送相关的信息
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump()); // response.dump()将json对象response序列化为字节流，通过send()将其发送给客户端
    }
    else
    {
        // 注册失败，向客户端发送相关的信息
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump()); // response.dump()将json对象response序列化为字节流，通过send()将其发送给客户端
    }
}

// 处理客户端异常退出业务
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        // 删除对应的连接信息
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 取消订阅
    _redis.unsubscribe(user.getId());

    // 更新用户状态信息：state改为offline
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 处理服务端异常退出业务
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}

// 处理点对点聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 提取接收方的id
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);

        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // 接收方连接能找到——在线，并且双方在同一台主机上，服务端将消息转发给接收方
            it->second->send(js.dump());
            return;
        }
    }
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        // 对方在线，说明在别的服务器上，则向Redis指定的通道channel发布消息
        _redis.publish(toid, js.dump());
    }
    else
    {
        // 接收方不在线，存储离线消息
        _offlineMsgModel.insert(toid, js.dump());
    }
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 如果要完善业务功能：验证要添加的好友是否存在

    // 添加好友信息
    User user = _userModel.query(friendid);
    if (user.getId() == -1)
    {
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "this friendid does not exist, please input another!";
        LOG_INFO << "Add friendid failed!!";
        conn->send(response.dump());        
    }
    else
    {
        _friendModel.insert(userid, friendid);
        _friendModel.insert(friendid, userid);
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["errno"] = 0;
        LOG_INFO << "Add friendid successful!!";
        conn->send(response.dump());           
    }
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 要创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        LOG_INFO << "Create group successful!!";
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
        
        // 创建成功，向客户端发送相关的信息
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 0;
        response["groupid"] = group.getId();
        conn->send(response.dump()); // response.dump()将json对象response序列化为字节流，通过send()将其发送给客户端        
    }
    else
    {
        LOG_INFO << "Create group failed!!";
        // 创建失败，向客户端发送相关的信息
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Create group failed!";
        conn->send(response.dump()); // response.dump()将json对象response序列化为字节流，通过send()将其发送给客户端          
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    // 验证该群是否存在
    Group group = _groupModel.query(groupid);
    if (group.getId() == -1)
    {
        // 加入群组失败
        LOG_INFO << "Add Group failed!!";

        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "this group does not exist, please input another!";
        conn->send(response.dump()); 
        return;          
    }

    // 若群存在，尝试加群
    if (_groupModel.addGroup(userid, groupid, "normal"))
    {
        // 加入群组成功
        LOG_INFO << "Add Group successful!!";

        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 0;
        response["errmsg"] = "Add Group successful!";              
        conn->send(response.dump());          
    }
    else
    {
        // 加入群组失败
        LOG_INFO << "Add Group failed!!";

        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Add Group failed!";        
        conn->send(response.dump());           
    }
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // vector<int> queryGroupUsers(int userid, int groupid);
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> vec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : vec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 接收方连接能找到——在线，并且双方在同一台主机上，服务端将消息转发给接收方
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                // 对方在线，说明在别的服务器上，则向Redis指定的通道channel发布消息
                _redis.publish(id, js.dump());
            }
            else
            {
                // 接收方不在线，存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的通道里的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    // 这里要理清楚逻辑，首先是服务器获取在redis中订阅的通道里的消息，然后服务器才将消息send()发送给指定的客户端。redis不是直接发送给客户端
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    else
    {
        //存储用户的离线消息，因为有可能服务器在获取订阅的通道里的消息时用户下线了
        _offlineMsgModel.insert(userid, msg);
    }
}