#include "../../../common/Detail.hpp"
#include "../../../client/Rpc_Client.hpp"

#include <thread>


void callback(const Json::Value &result)
{
    DLOG("result: %d",result.asInt());
}

int main()
{
    only_days::client::RpcClient client("127.0.0.1",9090,false);

    Json::Value params, result1,result2,result3;
    params["num1"] = 11;
    params["num2"] = 22;
    bool ret = client.call("Add",params,result1);
    if(ret != false)
    {
        cout<< " result1: "<<result1.asInt() << endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //异步
    only_days::client::RpcCaller::JsonAsyncResponse res_future;
    params["num1"] = 22;
    params["num2"] = 66;
    bool ret1 = client.call("Add",params,res_future);
    if(ret1 != false)
    {
        result2 = res_future.get();
        cout<< " result2: "<<result2.asInt() << endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    //回调函数
    params["num1"] = 22;
    params["num2"] = 77;
    bool ret2 = client.call("Add",params,callback);
    if(ret2 == false)
    {
        cout<< " result3 error "<< endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
}