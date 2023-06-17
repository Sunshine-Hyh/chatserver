#include "groupmodel.hpp"
#include "db.hpp"
#include <vector>

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    // 2.向allgroup表中插入记录
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 插入成功则获取用户数据的id
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(groupid, userid, grouprole) values(%d, %d, '%s')", groupid, userid, role.c_str());

    // 2.向groupuser表中插入记录
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    // 1. 首先根据userid在groupuser表中查询出所属的群组信息
    // 2. 根据所属群组查询属于该群组的所有用户的userid，更具userid和user表进行联合查询，查出用户的详细信息
    vector<Group> vec_Group;

    // 1.  首先根据userid在groupuser表中查询出所属的群组信息
    // 1.1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d", userid);

    // 1.2 向allgroup表中查询group的信息
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        MYSQL_ROW row;
        while (res != nullptr)
        {
            // 获取userid所属的群组信息
            row = mysql_fetch_row(res); // 获取一行的记录
            if (row != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec_Group.push_back(group);
            }
            else
                break;
        }
        mysql_free_result(res); // 释放动态内存
    }
    // 2. 根据所属群组查询属于该群组的所有用户的userid，根据userid和user表进行联合查询，查出用户的详细信息
    // 完善group类中的user成员变量
    for (Group &group : vec_Group)
    {
        // 向user表中查询groupuser的信息
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on a.id = b.userid where b.groupid = %d", group.getId());
        MYSQL_RES *res = mysql.query(sql);
        MYSQL_ROW row;
        while (res != nullptr)
        {
            // 获取userid所属的群组下的groupuser信息
            row = mysql_fetch_row(res); // 获取一行的记录
            if (row != nullptr)
            {
                GroupUser groupUser;
                groupUser.setId(atoi(row[0]));
                groupUser.setName(row[1]);
                groupUser.setState(row[2]);
                groupUser.setRole(row[3]);
                group.getUser().push_back(groupUser);
            }
            else
                break;
        }
        mysql_free_result(res); // 释放动态内存
    }

    // 查询群组的用户信息
    return vec_Group;
}

// 根据指定的groupid查询群组用户id列表(除了userid自己)，用户根据用户列表给成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    vector<int> vec;
    // 根据groupid到groupuser表中查找该群下所有的userid。
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        MYSQL_ROW row;
        while (res != nullptr)
        {
            // 获取userid所属的群组下的groupuser信息
            row = mysql_fetch_row(res); // 获取一行的记录
            if (row != nullptr)
            {
                vec.emplace_back(atoi(row[0]));
            }
            else
                break;
        }
        mysql_free_result(res); // 释放动态内存
    }
    return vec;
}