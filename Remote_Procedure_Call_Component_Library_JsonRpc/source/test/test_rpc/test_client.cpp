#include "../../common/Message.hpp"
#include "../../common/Net.hpp"

#include <thread>

void onMassage(const only_days::BaseConnection::ptr &conn,only_days::BaseMessage::ptr &msg)
{
    std::string body = msg->serialize();
    cout<<"1"<<endl;
    cout << body << endl;
}

int main()
{
    auto client = only_days::MuduoClientFactory::create("127.0.0.1",9090);
    client->setMessageCallback(onMassage);
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