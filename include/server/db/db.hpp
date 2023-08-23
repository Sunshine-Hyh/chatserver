#ifndef DB_H
#define DB_H
// 实现数据库的增删改查操作

#include <mysql/mysql.h>
#include <string>
#include <ctime>
using namespace std;

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();

    // 释放数据库连接资源
    ~MySQL();

    // 连接数据库
    bool connect(string ip, unsigned short port, string user, string password, string dbname);

    // 更新操作
    bool update(string sql);

    // 查询操作：针对select
    MYSQL_RES *query(string sql);

    // 获取连接
    MYSQL *getConnection();

    // 刷新一下连接的起始空闲时间点
    void refreshAliveTime();

    // 返回存活的时间(连接在连接池中处于空闲状态的时长：当前时间 - _aliveTime)
    clock_t getAliveTime() const;

private:
    MYSQL *_conn;       // 表示和MySQL Server的一条连接
    clock_t _aliveTime; // 记录连接进入空闲状态时的时间点
};
#endif