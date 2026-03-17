#include "../../common/Message.hpp"
#include "../../common/Net.hpp"
#include "../../common/Dispatcher.hpp"
#include "../../client/Requestor.hpp"
#include "../../client/RpcCaller.hpp"

#include <thread>


void callback(const Json::Value &result)
{
    DLOG("result: %d",result.asInt());
}

int main()
{
    auto requestor = std::make_shared<only_days::client::Requestor>();
    auto caller = std::make_shared<only_days::client::RpcCaller>(requestor);

    auto dispatcher = std::make_shared<only_days::Dispatcher>();
    auto rsp_cb = bind(&only_days::client::Requestor::onResponse,requestor.get()
    , std::placeholders::_1, std::placeholders::_2);
    //为什么这里使用RpcResponse类型就会报错，而使用BaseMessage就没问题了
    //这是因为在dispatcher模块中，你注册的是一个RpcResponse类型的指针
    //而requestor里面的onresponse的指针是BaseMessage类型的，所以这里只能用BaseMessage类型来解决问题
    dispatcher->registerHandler<only_days::BaseMessage>(only_days::MType::RSP_RPC,rsp_cb);
    

    auto client = only_days::MuduoClientFactory::create("127.0.0.1",9091);
    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,dispatcher.get(),\
    std::placeholders::_1,std::placeholders::_2);
    client->setMessageCallback(message_cb);
    client->connect();

    auto conn = client->connection();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    Json::Value params, result;
    
    //同步
    // params["num1"] = 11;
    // params["num2"] = 22;
    // bool ret = caller->call(conn,"Add",params,result);
    // if(ret != false)
    // {
    //     cout<< " result: "<<result.asInt() << endl;
    // }
    
    //异步
    // only_days::client::RpcCaller::JsonAsyncResponse res_future;
    // params["num1"] = 22;
    // params["num2"] = 66;
    // bool ret = caller->call(conn,"Add",params,res_future);
    // if(ret != false)
    // {
    //     result = res_future.get();
    //     cout<< " result: "<<result.asInt() << endl;
    // }

    //回调函数
    params["num1"] = 22;
    params["num2"] = 77;
    bool ret = caller->call(conn,"Add",params,callback);
    if(ret != false)
    {
        cout<< " result: "<<result.asInt() << endl;
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    client->shutdown();
    return 0;
}