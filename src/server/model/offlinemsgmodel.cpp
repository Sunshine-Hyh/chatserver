#include "offlinemessagemodel.hpp"
#include "dbConnectionPool.hpp"

// 向offlinemessage表格中添加离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage(userid, message) values(%d, '%s')", userid, msg.c_str());

    // 2.向数据库中插入离线消息
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    mysql->update(sql);
}

// 从offlinemessage表格中删除离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);

    // 2.从数据库中删除离线消息
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    mysql->update(sql);
}

// 从offlinemessage表格中查询离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    vector<string> vec;
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    // 2.向数据库中查询记录
    shared_ptr<MySQL> mysql = ConnectionPool::getConnectionPool()->getConnection();
    MYSQL_RES *res = mysql->query(sql);
    MYSQL_ROW row;
    while (res != nullptr)
    {
        // 把userid用户的所有离线消息放入vec中返回
        row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            vec.push_back(row[0]);
        }
        else
            break;
    }
    mysql_free_result(res);
    return vec;
}