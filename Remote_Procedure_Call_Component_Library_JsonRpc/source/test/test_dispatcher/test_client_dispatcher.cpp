#include "../../common/Message.hpp"
#include "../../common/Net.hpp"
#include "../../common/Dispatcher.hpp"

#include <thread>

void onRpcResponse(const only_days::BaseConnection::ptr &conn,only_days::RpcResponse::ptr &msg)
{
    cout<<"收到了一个Rpc响应"<<endl;
    std::string body = msg->serialize();
    cout << body << endl;
}

void onTopicResponse(const only_days::BaseConnection::ptr &conn,only_days::TopicResponse::ptr &msg)
{
    cout<<"收到了一个Topic响应"<<endl;
    std::string body = msg->serialize();
    cout << body << endl;
}

int main()
{
    auto dispatcher = std::make_shared<only_days::Dispatcher>();
    dispatcher->registerHandler<only_days::RpcResponse>(only_days::MType::RSP_RPC,onRpcResponse);
    dispatcher->registerHandler<only_days::TopicResponse>(only_days::MType::RSP_TOPIC,onTopicResponse);

    auto client = only_days::MuduoClientFactory::create("127.0.0.1",9091);
    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,dispatcher.get(),\
    std::placeholders::_1,std::placeholders::_2);
    client->setMessageCallback(message_cb);
    client->connect();

    auto rpc_req = only_days::MessageFactory::create<only_days::RpcRequest>();
    rpc_req->setId("2222222");
    rpc_req->setMType(only_days::MType::REQ_RPC);
    rpc_req->setMethod("Add");
    Json::Value params;
    params["num1"] = 11;
    params["num2"] = 22;
    rpc_req->setParams(params);

    client->send(rpc_req);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    client->shutdown();
    return 0;
}