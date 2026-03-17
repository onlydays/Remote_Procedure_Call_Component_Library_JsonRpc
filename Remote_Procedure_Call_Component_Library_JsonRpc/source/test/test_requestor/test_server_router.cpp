#include "../../common/Message.hpp"
#include "../../common/Net.hpp"
#include "../../common/Dispatcher.hpp"
#include "../../server/Rpc_Router.hpp"

void Add(const Json::Value &req,Json::Value &rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main()
{
    auto dispatcher = std::make_shared<only_days::Dispatcher>();
    auto router = std::make_shared<only_days::server::RpcRouter>();
    std::unique_ptr<only_days::server::SDescribeFactory> desc_factory(new only_days::server::SDescribeFactory());
    desc_factory->setMethodName("Add");
    desc_factory->setParamsDesc("num1",only_days::server::VType::INTEGRAL); 
    desc_factory->setParamsDesc("num2",only_days::server::VType::INTEGRAL); 
    desc_factory->setReturnType(only_days::server::VType::INTEGRAL);
    desc_factory->setCallback(Add);
    router->registerMethod(desc_factory->build());

    auto cb = std::bind(&only_days::server::RpcRouter::onRpcRequest,router.get()
    , std::placeholders::_1, std::placeholders::_2);
    dispatcher->registerHandler<only_days::RpcRequest>(only_days::MType::REQ_RPC,cb);

    auto server = only_days::MuduoServerFactory::create(9091);
    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,dispatcher.get(),\
    std::placeholders::_1,std::placeholders::_2);
    server->setMessageCallback(message_cb);
    server->start();
    return 0;
}