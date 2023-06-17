#ifndef USER_H
#define USER_H

// 根据数据库中User表设计User类
#include <string>
using namespace std;

class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline") : id(id), name(name), password(pwd), state(state)
    {
        // 当新增用户时候，注册新用户时候，数据库对主键id自增
    }
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }

private:
    int id;
    string name;
    string password;
    string state;
};

#endif