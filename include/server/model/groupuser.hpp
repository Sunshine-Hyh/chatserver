#ifndef GROUPUSER_H
#define GroupUSER_H

#include "user.hpp"

// 群组用户，继承自User类，多了一个role角色信息
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    string role;
};

#endif