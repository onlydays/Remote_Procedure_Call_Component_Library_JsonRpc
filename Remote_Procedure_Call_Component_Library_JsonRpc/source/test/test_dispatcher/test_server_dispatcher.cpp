#include "../../common/Message.hpp"
#include "../../common/Net.hpp"
#include "../../common/Dispatcher.hpp"

void onRpcRequest(const only_days::BaseConnection::ptr &conn,only_days::RpcRequest::ptr &msg)
{
    cout<<"收到了一个Rpc请求"<<endl;
    cout<<"方法名称："<<msg->method()<<endl;
    std::string body = msg->serialize();
    cout << body << endl;
    auto rpc_rsp = only_days::MessageFactory::create<only_days::RpcResponse>();
    rpc_rsp->setId("11111111");
    rpc_rsp->setMType(only_days::MType::RSP_RPC);
    rpc_rsp->setRcode(only_days::RCode::RCODE_OK);
    rpc_rsp->setResult(33);
    conn->send(rpc_rsp);
}

void onTopicRequest(const only_days::BaseConnection::ptr &conn,only_days::TopicRequest::ptr &msg)
{
    cout<<"收到了一个Topic请求"<<endl;
    std::string body = msg->serialize();
    cout << body << endl;
    auto topic_rsp = only_days::MessageFactory::create<only_days::TopicResponse>();
    topic_rsp->setId("99999999");
    topic_rsp->setMType(only_days::MType::RSP_TOPIC);
    topic_rsp->setRcode(only_days::RCode::RCODE_OK);
    conn->send(topic_rsp);
}

int main()
{
    auto server = only_days::MuduoServerFactory::create(9091);
    auto dispatcher = std::make_shared<only_days::Dispatcher>();
    dispatcher->registerHandler<only_days::RpcRequest>(only_days::MType::REQ_RPC,onRpcRequest);
    dispatcher->registerHandler<only_days::TopicRequest>(only_days::MType::REQ_TOPIC,onTopicRequest);
    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,dispatcher.get(),\
    std::placeholders::_1,std::placeholders::_2);
    server->setMessageCallback(message_cb);
    server->start();
    return 0;
}