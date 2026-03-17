#include "../../../common/Detail.hpp"
#include "../../../server/Rpc_Server.hpp"

void Add(const Json::Value &req,Json::Value &rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main()
{
    std::unique_ptr<only_days::server::SDescribeFactory> desc_factory(new only_days::server::SDescribeFactory());
    desc_factory->setMethodName("Add");
    desc_factory->setParamsDesc("num1",only_days::server::VType::INTEGRAL); 
    desc_factory->setParamsDesc("num2",only_days::server::VType::INTEGRAL); 
    desc_factory->setReturnType(only_days::server::VType::INTEGRAL);
    desc_factory->setCallback(Add);
    
    only_days::server::RpcServer server(only_days::Address("127.0.0.1",9090),true,only_days::Address("127.0.0.1",8080));
    server.registerMethod(desc_factory->build());
    server.start();
    return 0;
}