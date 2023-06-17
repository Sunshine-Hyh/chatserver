#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
using namespace std;

#include "json.hpp"
using json = nlohmann::json; // 这里将作用域简短化成json

string func0()
{
    json js; // 生成一个json对象
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    string sendBuf = js.dump();  // 该函数作用是将json数据对象 → 序列化  json字符串，这样我们可以通过网络将其发送出去
    // cout << sendBuf.c_str() << endl;
    // {"from":"zhang san","msg":"hello, what are you doing now?","msg_type":2,"to":"li si"}
    return sendBuf;
}

void parse0()
{
    string recvBuf = func0();

    // 数据的反序列化 将json字符串  → 反序列化  json数据对象
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;

}

// json示例1：普通数据序列化
void func1()
{
    json js; // 生成一个json对象
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["app"] = {"webapp", "wechat"};

    // 我们可以直接打印序列化的字符串
    // {"app":["webapp","wechat"],"from":"zhang san","msg_type":2,"to":"li si"}
    cout << js << endl;
}

// json示例2：普通数据序列化
string func2()
{
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};


    // cout << js << endl;  
    // {"id":[1,2,3,4,5],"msg":{"liu shuo":"hello china","zhang san":"hello world"},"name":"zhang san"}
    string sendBuf = js.dump();
    return sendBuf;

}
void parse2()
{
    string recvBuf = func2();
    json jsbuf = json:: parse(recvBuf);

    cout << "-----------------------" << endl;
    cout << jsbuf["id"] << endl;
    auto arr = jsbuf["id"];
    cout << arr[3] << "\n" << endl;

    cout << jsbuf["name"] << endl;  // "zhang san"
    string name = jsbuf["name"];    
    cout << name << "\n"  << endl;  // zhang san

    cout << jsbuf["msg"] << endl;                  // {"liu shuo":"hello china","zhang san":"hello world"}
    cout << jsbuf["msg"]["zhang san"] << endl;     // "hello world"
    string zhangsan = jsbuf["msg"]["zhang san"];   // hello world
    cout << zhangsan << endl;    
    auto msgjs = jsbuf["msg"];
    cout << msgjs["zhang san"] << endl;            // "hello world"
    cout << msgjs["liu shuo"] << "\n"  << endl;    // "hello china"
}

// json示例3：容器序列化
string func3()
{
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;

    cout<<js<<endl;
    // {"list":[1,2,5],"path":[[1,"黄山"],[2,"华山"],[3,"泰山"]]}

    string sendBuf = js.dump();
    // cout << sendBuf << endl;
    // {"list":[1,2,5],"path":[[1,"黄山"],[2,"华山"],[3,"泰山"]]}

    return sendBuf;
}

void parse3()
{
    string recvBuf = func3();
    json jsbuf = json:: parse(recvBuf);

    vector<int> vec = jsbuf["list"];
    for (int op : vec) cout << op << " ";
    cout << endl;

    map<int, string> m = jsbuf["path"];
    for (pair<int, string> op : m) cout << op.first << " " << op.second << endl;

}



int main()
{
    // func0();
    // func1();
    // func2();
    // func3();

    // parse2();
    parse3();

    return 0;
}
