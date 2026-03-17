#include "../../common/Message.hpp"
#include "../../common/Net.hpp"

void onMassage(const only_days::BaseConnection::ptr &conn,only_days::BaseMessage::ptr &msg)
{
    std::string body = msg->serialize();
    cout<<"2"<<endl;
    cout << body << endl;
    auto rpc_rsp = only_days::MessageFactory::create<only_days::RpcResponse>();
    rpc_rsp->setId("11111111");
    rpc_rsp->setMType(only_days::MType::RSP_RPC);
    rpc_rsp->setRcode(only_days::RCode::RCODE_OK);
    rpc_rsp->setResult(33);
    conn->send(rpc_rsp);
}

int main()
{
    auto server = only_days::MuduoServerFactory::create(9090);
    server->setMessageCallback(onMassage);
    server->start();
    return 0;
}