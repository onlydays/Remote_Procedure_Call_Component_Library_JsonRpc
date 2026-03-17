#include "../../client/Rpc_Client.hpp"

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
    //3. 向主题发布消息
    for(int i = 0;i<10;i++)
    {
        client->publish(topic_name,"hello world - " + std::to_string(i));
    }
    client->shutdown();
    return 0;
}