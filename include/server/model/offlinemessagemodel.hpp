#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <vector>
#include <string>

using namespace std;

// 对离线消息表格offlinemessage数据操作类
class OfflineMsgModel
{
public:
    // 向offlinemessage表格中添加数据
    void insert(int userid, string msg);

    // 向offlinemessage表格中移出数据
    void remove(int userid);   

    // 从offlinemessage表格中查询数据
    vector<string> query(int userid);         

};
#endif