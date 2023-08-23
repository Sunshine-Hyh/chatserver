#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <ctime>
#include <chrono>
#include <algorithm>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "user.hpp"
#include "group.hpp"
#include "public.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;

// 启动退出程序时，不再执行mainMenu的相关程序
bool mainMenuisrunning = false;

// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;

// 记录当前用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 记录当前用户的离线列表信息
vector<string> g_currentUserOfflineMessageList;

// 用于读写线程之间的通信
sem_t rwsem;

// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程：接收线程(接收服务器发过来的数据)
void readTaskHandler(int clientfd);

// 获取系统时间(聊天信息需要添加时间信息)
string getCurrentTime();

// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现，main线程用作发送线程(接受用户输入)，子线程用作接收线程(接收服务器发过来的数据)
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 192.168.37.128 6000" << endl;
        exit(-1);
    }

    // 1.客户端与服务端之间建立连接
    // 1.1 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]); // typedef signed short int int16_t;//给有符号短整型short int，取别名int16_t，int16_t 表示数据范围为-32768~32767

    // 1.2 创建客户端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 1.3 client需要连接的server信息：ip + port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 1.4 client和server建立连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 客户端与服务器之间连接成功，启动子线程专门用于接收服务端消息
    std::thread readTask(readTaskHandler, clientfd); // C++11提供的线程库，底层对pthread进行封装，因为readTaskHandler需要一个clientfd，所以传递一个readTaskHandler
    readTask.detach();                               // 线程分离，当线程运行完毕自动回收线程 pthread_detach()

    // 2. 客户端与服务端之间进行通信
    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单：登录、注册、退出
        cout << "=====================================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "=====================================" << endl;
        cout << "choice:";

        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车字符，防止下一次读取字符串将其读取

        switch (choice)
        {
        case 1: // login业务
        {
            // 服务端处理登录业务需要提供：id  password
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车字符，防止下一次读取字符串将其读取
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump(); // 将json对象js序列化为字符串

            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {

                // 5. 登录成功，启动接收线程负责接收数据，该线程只启动一次(当退出登录时候，已经与服务端断开连接，recv函数是阻塞的，除非重新登陆建立连接)
                //(前面的：用户信息 + 群组信息 + 好友列表相当于是客户端登录后服务器主动反馈，接下来才开始客户段登录服务器后面的聊天、加好友、加群等操作)
                // static int readthreadnum = 0;  // 退出登录时候，不能再启动线程
                // if (readthreadnum == 0)
                // {
                //     std::thread readTask(readTaskHandler, clientfd); // C++11提供的线程库，底层对pthread进行封装，因为readTaskHandler需要一个clientfd，所以传递一个readTaskHandler
                //     readTask.detach();                               // 线程分离，当线程运行完毕自动回收线程 pthread_detach()
                //     ++ readthreadnum;
                // }
                sem_wait(&rwsem);   // 等待信号量，由子线程处理完登录的响应消息后，通知这里

                if (g_isLoginSuccess)
                {
                    // 6. 进入聊天主菜单界面
                    mainMenuisrunning = true;
                    mainMenu(clientfd);
                }
            }
            break;
        }
        case 2: // register业务
        {
            // 服务端处理注册业务需要提供：name  password
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);  //cin >> 遇到空格回车等会停止读取，因此使用cin.getline()
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump(); // 将json对象js序列化为字符串

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                sem_wait(&rwsem);   // 等待信号量，由子线程处理完注册的响应消息后，通知这里
            }
            break;
        }
        case 3: // quit业务
        {
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
    return 0;
}

void doLoginResponse(json &responsejs)
{
    // 接收服务端发送过来的数据
    if (0 != responsejs["errno"].get<int>())
    {
        // 登录失败，输出错误信息(已经登陆/id或者密码错误)
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else
    {
        // 登录成功
        // 服务端：更新用户的状态信息 + 查询是否有离线消息 + 返回好友列表

        // 1. 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 2. 记录当前登录用户的好友列表信息vector<User> g_currentUserFriendList;
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear(); // 这里是为了调用loginout()后回到首页面防止再次读取当前用户的相关信息导致输出双份信息(首次登录时已经存储了一次)
            vector<string> vec = responsejs["friends"];
            for (string str : vec)
            {
                User user;
                json js = json::parse(str);
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 3. 记录当前用户的群组列表信息：vector<Group> g_currentUserGroupList;
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();
            vector<string> vec_group = responsejs["groups"];
            for (string str : vec_group)
            {
                Group group;
                json js = json::parse(str);
                group.setId(js["id"].get<int>());
                group.setName(js["groupname"]);
                group.setDesc(js["groupdesc"]);

                vector<string> vec_user = js["users"];
                for (string str_user : vec_user)
                {
                    GroupUser user;
                    json js = json::parse(str_user);
                    user.setId(js["id"]);
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUser().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }


        // 4. 显示当前用户的离线消息： 个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            // 初始化
            g_currentUserOfflineMessageList.clear();
            vector<string> vec_offlinemessage = responsejs["offlinemsg"];
            for (string str : vec_offlinemessage)
            {
                g_currentUserOfflineMessageList.push_back(str);
            }
            // for (string str : vec_offlinemessage)
            // {
            //     json js = json::parse(str);
            //     // time + [id] + name + " said " + xxx
            //     if (ONE_CHAT_MSG == js["msgid"].get<int>())
            //     {
            //         // 一对一离线聊天消息
            //         cout << "一对一消息: " << js["time"] << " [" << js["id"] << "]" << js["name"] << " said: " << js["msg"] << endl;
            //     }
            //     else
            //     {
            //         cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
            //              << " said: " << js["msg"].get<string>() << endl;
            //     }
            // }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();
        g_isLoginSuccess = true;    // 登录成功
    }
}

void doRegResponse(json &responsejs)
{
    // 接收服务端发送过来的数据
    if (0 != responsejs["errno"].get<int>())
    {
        // 注册失败
        cerr << "name is already exist, register error!" << endl;
    }
    else
    {
        // 注册成功
        cout << "register successful, userid is " << responsejs["id"] << ", do not forget it!" << endl;
    }    
}

void doAddFriendResponse(json &responsejs)
{
    // 接收服务端发送过来的数据
    if (0 != responsejs["errno"].get<int>())
    {
        // 添加好友失败
        cerr << responsejs["errmsg"] << endl;
    }
    else
    {
        // 添加好友成功
        cout << "Add friendid successful!" << endl;
    }    
}

void doCreateGroupResponse(json &responsejs)
{
    // 接收服务端发送过来的数据
    if (0 != responsejs["errno"].get<int>())
    {
        // 创建群组失败
        cerr << responsejs["errmsg"] << endl;
    }
    else
    {
        // 创建群组成功
        cout << "Create group successful!\tGroupID : " << responsejs["groupid"] << endl;
    }    
}

void doAddGroupResponse(json &responsejs)
{
    // 接收服务端发送过来的数据
    if (0 != responsejs["errno"].get<int>())
    {
        // 加入群组失败
        cerr << responsejs["errmsg"] << endl;
    }
    else
    {
        // 加入群组成功
        cout << "Add Group successful!" << endl;
    }    
}

// 子线程 - 接收线程函数：接收线程(接收服务器发过来的数据)
void readTaskHandler(int clientfd)
{
    // 接收服务端ChatServer发送过来的数据
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        // 一对一聊天：接收ChatServer转发的数据，反序列化成json对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << "一对一消息: " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (msgtype == GROUP_CHAT_MSG)
        {
            cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (msgtype == LOGIN_MSG_ACK)
        {
            // 处理登陆响应的业务逻辑
            doLoginResponse(js);
            sem_post(&rwsem);   // 通知主线程，登录结果处理完成
            continue;
        }
        else if (msgtype == REG_MSG_ACK)
        {
            // 处理注册响应的业务逻辑
            doRegResponse(js);
            sem_post(&rwsem);   // 通知主线程，注册结果处理完成
            continue;          
        }
        else if (msgtype == ADD_FRIEND_MSG_ACK)
        {
            // 处理添加好友响应的业务逻辑
            doAddFriendResponse(js);
            continue;          
        }  
        else if (msgtype == CREATE_GROUP_MSG_ACK)
        {
            // 处理创建群组响应的业务逻辑
            doCreateGroupResponse(js);
            continue;          
        }   
        else if (msgtype == ADD_GROUP_MSG_ACK)
        {
            // 处理加入群组响应的业务逻辑
            doAddGroupResponse(js);
            continue;          
        }                      
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "===========================login user===========================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "---------------------------friend list---------------------------" << endl;

    if (!g_currentUserFriendList.empty())
    {
        for (User user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    else
        cout << "Your FriendList is empty!\n" << endl;


    cout << "----------------------------group list----------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser user : group.getUser())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    else 
        cout << "Your GroupList is empty!\n" << endl;    
    cout << "============================offlinemsg============================" << endl;
    if (!g_currentUserOfflineMessageList.empty())
    {
        for (string str : g_currentUserOfflineMessageList)
        {
            json js = json::parse(str);
            // time + [id] + name + " said " + xxx
            if (ONE_CHAT_MSG == js["msgid"].get<int>())
            {
                // 一对一离线聊天消息
                cout << "一对一消息: " << js["time"] << " [" << js["id"] << "]" << js["name"] << " said: " << js["msg"] << endl;
            }
            else
            {
                cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                        << " said: " << js["msg"].get<string>() << endl;
            }
        }
    }   
    else
        cout << "Your OfflinemessageList is empty!\n" << endl;  
    cout << "=======================================================================" << endl;           
}

// "help"command handler
void help(int fd = 0, string str = "");

// "chat"command handler
void chat(int, string);

// "addfriend"command handler
void addfriend(int, string);

// "creategroup"command handler
void creategroup(int, string);

// "addgroup"command handler
void addgroup(int, string);

// "groupchat"command handler
void groupchat(int, string);

// "loginout"command handler
void loginout(int fd = 0, string str = "");

// 系统支持的客户端；命令列表
unordered_map<string, string> commandMap = {
    {"help:", "\t\t显示所有支持的命令\t格式  help"},
    {"chat:", "\t\t一对一聊天\t\t格式  chat:friendid:message"},
    {"addfriend:", "\t添加好友\t\t格式  addfriend:friendid"},
    {"creategroup:", "\t创建群组\t\t格式  creategroup:groupname:groupdesc"},
    {"addgroup:", "\t加入群组\t\t格式  addgroup:groupid"},
    {"groupchat:", "\t群聊\t\t\t格式  groupchat:groupid:message"},
    {"loginout:", "\t注销\t\t\t格式  loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (mainMenuisrunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 从输入中提取要客户要调用的命令
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            // 说明命令是help/loginout
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }

        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        else
        {
            // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
            it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx - 1));
        }
    }
}

// "help"command handler
void help(int fd, string str)
{
    cout << "show command list >>>" << endl;
    for (auto op : commandMap)
    {
        cout << op.first << " " << op.second << endl;
    }
    cout << endl;
}

// "chat"command handler
void chat(int clientfd, string str)
{
    // "一对一聊天, 格式chat:friendid:message"  这里str = "friendid:message"
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = stoi(str.substr(0, idx));
    string message = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg error -> " << buf << endl;
    }
}

// "addfriend"command handler
void addfriend(int clientfd, string str)
{
    // 添加好友格式：addfriend:friendid str = "friendid"
    int friendid = stoi(str);
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buf = js.dump();
    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addfriend msg error -> " << buf << endl;
    }
}

// "creategroup"command handler
void creategroup(int clientfd, string str)
{
    // "创建群组, 格式creategroup:groupname:groupdesc"
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send creategroup msg error -> " << buf << endl;
    }
}

// "addgroup"command handler
void addgroup(int clientfd, string str)
{
    // "addgroup", "加入群组, 格式addgroup:groupid"
    int groupid = stoi(str);

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error -> " << buf << endl;
    }
}

// "groupchat"command handler
void groupchat(int clientfd, string str)
{
    // "groupchat", "群聊, 格式groupchat:groupid:message"
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = stoi(str.substr(0, idx));
    string message = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupchat msg error -> " << buf << endl;
    }
}

// "loginout"command handler
void loginout(int clientfd, string str)
{
    // "loginout", "注销, 格式loginout"
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();

    string buf = js.dump();

    int len = send(clientfd, buf.c_str(), strlen(buf.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send loginout msg error -> " << buf << endl;
    }
    else
        mainMenuisrunning = false;
}
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}