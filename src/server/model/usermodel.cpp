#include "usermodel.hpp"
#include "user.hpp"
#include "dbConnectionPool.hpp"


// User表的插入方法
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    // c_str()就是将C++的string转化为C的字符串数组
    // MySQL语法中要插入的数据是字符串时候要加上''，因此是'%s'而不是%s

    // 2.向数据库中插入记录
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    if (mysql->update(sql))
    {
        // 插入成功则获取用户数据的id
        user.setId(mysql_insert_id(mysql->getConnection()));
        return true;
    }
    return false;
}

// User表的查询方法
User UserModel::query(int id)
{
    User user;
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    // 2.向数据库中查询记录
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    MYSQL_RES *res = mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res); // 获取一行的记录
        if (row != nullptr)
        {
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res); // 释放动态内存
            return user;
        }
    }
    return user;
}

// 更新用户的状态信息
bool UserModel::updateState(User user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    // 2.向数据库中更新记录
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    if (mysql->update(sql))
    {
        user.setId(mysql_insert_id(mysql->getConnection()));
        return true;
    }
    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    // 1.组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    // 2.向数据库中更新记录
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    mysql->update(sql);
}