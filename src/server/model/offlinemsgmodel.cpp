#include "offlinemessagemodel.hpp"

// 向offlinemessage表格中添加离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage(userid, message) values(%d, '%s')", userid, msg.c_str());

    // 2.向数据库中插入离线消息
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 从offlinemessage表格中删除离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);

    // 2.从数据库中删除离线消息
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 从offlinemessage表格中查询离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    vector<string> vec;
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    // 2.向数据库中查询记录
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        // 视频代码
        // if (res != nullptr)
        // {
        //     MYSQL_ROW row;
        //     // 把userid用户的所有离线消息放入vec中返回
        //     while ((row = mysql_fetch_row(res)) != nullptr) 
        //     {
        //         vec.push_back(row[0]);
        //     }
        // }
        MYSQL_ROW row;
        while (res != nullptr)
        {            
            // 把userid用户的所有离线消息放入vec中返回
            row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                vec.push_back(row[0]);
            }
            else break;
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}