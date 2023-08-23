#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include "group.hpp"

// 维护群组信息的操作类
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    bool addGroup(int userid, int groupid, string role);

    // 根据id查询群组的信息
    Group query(int groupid);    

    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);

    // 根据指定的groupid查询群组用户id列表(除了userid自己)，用户根据用户列表给成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif