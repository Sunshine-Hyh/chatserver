#include "db.hpp"
#include <muduo/base/Logging.h>

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    // 开辟一块内存用于与数据库建立连接
    _conn = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // 连接成功，C和C++代码默认的编码字符是ASCII，如果不设置则数据库拉下来的中文信息会乱码
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!!";
    }
    else
    {
        LOG_INFO << "connect mysql failed!!";
    }
    return p;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!";
        return false;
    }
    else
    {
        return true;
    }
}

// 查询操作：针对select
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL *MySQL::getConnection()
{
    return this->_conn;
}
