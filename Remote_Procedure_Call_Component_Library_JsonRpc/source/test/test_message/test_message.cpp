#include "../../common/Message.hpp"

using std::cout;
using std::endl;

void TestRpcRequest()
{
    only_days::RpcRequest::ptr rrp = only_days::MessageFactory::create<only_days::RpcRequest>();
    Json::Value params;
    params["num1"] = 11;
    params["num2"] = 22;
    rrp->setMethod("Add");
    rrp->setParams(params);
    std::string str = rrp->serialize();
    cout << str << endl;

    only_days::BaseMessage::ptr bmp = only_days::MessageFactory::create(only_days::MType::REQ_RPC);
    bool ret = bmp->unserialize(str);
    if (ret == false)
    {
        exit(-1);
    }
    ret = bmp->check();
    if (ret == false)
    {
        exit(-1);
    }
    only_days::RpcRequest::ptr rrp2 = std::dynamic_pointer_cast<only_days::RpcRequest>(bmp);
    cout << rrp2->method() << endl;
    cout << rrp2->params()["num1"].asInt() << endl;
    cout << rrp2->params()["num2"].asInt() << endl;
}
void TestTopicRequest()
{
    only_days::TopicRequest::ptr trp = only_days::MessageFactory::create<only_days::TopicRequest>();
    trp->setTopicKey("news");
    trp->setTopicMessage("hello world!");
    trp->setTopicOptype(only_days::TopicOptype::TOPIC_PUBLISH);
    std::string str = trp->serialize();
    cout << str << endl;

    only_days::BaseMessage::ptr bmp = only_days::MessageFactory::create(only_days::MType::REQ_TOPIC);
    bool ret = bmp->unserialize(str);
    if (ret == false)
    {
        exit(-1);
    }
    ret = bmp->check();
    if (ret == false)
    {
        exit(-1);
    }
    only_days::TopicRequest::ptr trp2 = std::dynamic_pointer_cast<only_days::TopicRequest>(bmp);
    cout<<trp2->topicKey()<<endl;
    cout<<(int)trp2->topicOptype()<<endl;
}
void TestServiceRequest()
{
    only_days::ServiceRequest::ptr srp = only_days::MessageFactory::create<only_days::ServiceRequest>();
    srp->setServiceOptype(only_days::ServiceOptype::SERVICE_REGISTRY);
    srp->setMethod("Add");
    srp->setServiceHost(only_days::Address("127.0.0.1",9090));
    std::string str = srp->serialize();
    cout << str << endl;

    only_days::BaseMessage::ptr bmp = only_days::MessageFactory::create(only_days::MType::REQ_SERVICE);
    bool ret = bmp->unserialize(str);
    if (ret == false)
    {
        exit(-1);
    }
    ret = bmp->check();
    if (ret == false)
    {
        exit(-1);
    }
    only_days::ServiceRequest::ptr srp2 = std::dynamic_pointer_cast<only_days::ServiceRequest>(bmp);
    cout<<srp2->method()<<endl;
    cout<<srp2->serviceHost().first<<srp2->serviceHost().second<<endl;
    cout<<(int)srp2->serviceOptype()<<endl;
}
void TestRpcResponse()
{
    only_days::RpcResponse::ptr rrp = only_days::MessageFactory::create<only_days::RpcResponse>();
    rrp->setRcode(only_days::RCode::RCODE_OK);
    rrp->setResult(33);
    std::string str = rrp->serialize();
    cout << str << endl;

    only_days::BaseMessage::ptr bmp = only_days::MessageFactory::create(only_days::MType::RSP_RPC);
    bool ret = bmp->unserialize(str);
    if (ret == false)
    {
        exit(-1);
    }
    ret = bmp->check();
    if (ret == false)
    {
        exit(-1);
    }
    only_days::RpcResponse::ptr rrp2 = std::dynamic_pointer_cast<only_days::RpcResponse>(bmp);
    cout<<(int)rrp2->rcode()<<endl;
    cout<<rrp2->result()<<endl;
}
void TestTopicResponse()
{
    only_days::TopicResponse::ptr trp = only_days::MessageFactory::create<only_days::TopicResponse>();
    trp->setRcode(only_days::RCode::RCODE_OK);
    std::string str = trp->serialize();
    cout << str << endl;

    only_days::BaseMessage::ptr bmp = only_days::MessageFactory::create(only_days::MType::RSP_TOPIC);
    bool ret = bmp->unserialize(str);
    if (ret == false)
    {
        exit(-1);
    }
    ret = bmp->check();
    if (ret == false)
    {
        exit(-1);
    }
    only_days::TopicResponse::ptr trp2 = std::dynamic_pointer_cast<only_days::TopicResponse>(bmp);
    cout<<(int)trp2->rcode()<<endl;
}
void TestServiceResponse()
{
    only_days::ServiceResponse::ptr srp = only_days::MessageFactory::create<only_days::ServiceResponse>();
    srp->setMethod("Add");
    srp->setRcode(only_days::RCode::RCODE_OK);
    srp->setServiceOptype(only_days::ServiceOptype::SERVICE_DISCOVERY);
    std::vector<only_days::Address> addrs;
    addrs.push_back(only_days::Address("127.0.0.1",9090));
    addrs.push_back(only_days::Address("127.0.0.2",9091));
    addrs.push_back(only_days::Address("127.0.0.3",9092));
    srp->setServiceHost(addrs);
    std::string str = srp->serialize();
    cout << str << endl;

    only_days::BaseMessage::ptr bmp = only_days::MessageFactory::create(only_days::MType::RSP_SERVICE);
    bool ret = bmp->unserialize(str);
    if (ret == false)
    {
        exit(-1);
    }
    ret = bmp->check();
    if (ret == false)
    {
        exit(-1);
    }
    only_days::ServiceResponse::ptr srp2 = std::dynamic_pointer_cast<only_days::ServiceResponse>(bmp);
    cout<<srp2->method()<<endl;
    cout<<(int)srp2->rcode()<<endl;
    cout<<(int)srp2->serviceOptype()<<endl;
    std::vector<only_days::Address> addr1 = srp2->serviceHost();
    for(auto& e:addr1)
    {
        cout<<e.first<<":"<<e.second<<endl;
    }
}

int main()
{
    //TestRpcRequest();
    //TestTopicRequest();
    //TestServiceRequest();
    //TestRpcResponse();
    //TestTopicResponse();
    //TestServiceResponse();
    return 0;
}