#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "user.hpp"

// 维护好友信息表的类
class FriendModel
{
public:
    void insert(int userid, int friendid);

    vector<User> query(int userid);
};


#endif