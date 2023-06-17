#ifndef GROUP_H
#define GROUP_H

#include <string>
#include <vector>
#include "groupuser.hpp"

using namespace std;

// 对应allgroup表
class Group
{
public:
    Group(int id = -1, string name = "", string des = "") : id(id), name(name), desc(des)
    {
        // 当新增群时，注册新群，数据库对主键id自增
    }
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser> &getUser() { return this->users; }

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;
};

#endif