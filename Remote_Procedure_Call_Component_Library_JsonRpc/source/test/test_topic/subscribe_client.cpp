#include "../../client/Rpc_Client.hpp"
#include<thread>

void callback(const std::string& key, const std::string& msg)
{
    DLOG("%s主题收到推送过来的消息：%s",key.c_str(),msg.c_str());
}

int main()
{
    //1. 实例化客户端对象
    auto client = std::make_shared<only_days::client::TopicClient>("127.0.0.1",7070);
    //2. 创建主题
    std::string topic_name = "hello";
    bool ret = client->create(topic_name);
    if(ret == false)
    {
        DLOG("创建主题失败");
    }
    //3. 订阅主题
    ret = client->subscribe(topic_name,callback);
    //4.等待 -> 退出
    std::this_thread::sleep_for(std::chrono::seconds(20));
    client->shutdown();
    return 0;
}