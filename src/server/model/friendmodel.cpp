#include "friendmodel.hpp"
#include "db.hpp"

void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend(userid, friendid) values(%d, %d)", userid, friendid);

    // 2.向数据库中插入数据
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

vector<User> FriendModel::query(int userid)
{
    vector<User> vec;
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userid);

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
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                vec.push_back(user);
            }
            else break;
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;    
}