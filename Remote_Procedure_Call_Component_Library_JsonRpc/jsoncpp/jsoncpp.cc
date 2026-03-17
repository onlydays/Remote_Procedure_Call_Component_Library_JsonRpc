#include<iostream>
#include<sstream>
#include<string>
#include<memory.h>
#include<jsoncpp/json/json.h>

//实现数据的序列化
bool serialize(const Json::Value &val,std::string& content)
{
    std::stringstream ss;
    //先实例化一个工厂类对象
    Json::StreamWriterBuilder zby;
    //通过工厂类对象来生产派生对象
    std::unique_ptr<Json::StreamWriter> zb(zby.newStreamWriter());
    int ret = zb->write(val,&ss);
    if(ret != 0){
        std::cout<<"json serialize failed!\n";
        return false;
    }
    content = ss.str();
    return true;
}

// 实现json字符串的反序列化
bool unserialize(const std::string &body, Json::Value &val)
{
    // 实例化⼯⼚类对象
    Json::CharReaderBuilder crb;
    // ⽣产CharReader对象
    std::string errs;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), &val, &errs);
    if (ret == false)
    {
        std::cout << "json unserialize failed : " << errs << std::endl;
        return false;
    }
    return true;
}

int main()
{
    const char* name = "小明";
    int age = 20;
    const char* sex = "武装直升机";
    float score[3] = {88,93,53};

    Json::Value stu;
    stu["姓名"] = name;
    stu["年龄"] = age;
    stu["性别"] = sex;
    stu["语数英成绩"].append(score[0]); 
    stu["语数英成绩"].append(score[1]); 
    stu["语数英成绩"].append(score[2]); 
    //std::cout<<"姓名："<<stu["姓名"].asString()<<std::endl;

    Json::Value fav;
    fav["书籍"] = "西游记";
    fav["运动"] = "篮球";
    stu["爱好"] = fav;

    std::string content;
    serialize(stu,content);
    std::cout<<content<<std::endl;

    return 0;
}