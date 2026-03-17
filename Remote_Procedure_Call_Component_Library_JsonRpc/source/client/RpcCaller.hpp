#pragma once
#include "Requestor.hpp"
#include "../common/Detail.hpp"

namespace only_days
{
    namespace client
    {
        class RpcCaller
        {
        public:
            using ptr = std::shared_ptr<RpcCaller>;
            using JsonAsyncResponse = std::future<Json::Value>;
            using JsonResponseCallback = std::function<void(const Json::Value &)>;

            RpcCaller(const Requestor::ptr &requestor) 
            : _requestor(requestor) 
            {}

            //同步
            bool call(const BaseConnection::ptr &conn, const std::string &method, const Json::Value &params, Json::Value &result)
            {
                DLOG("开始同步rpc调用...");
                //1.组织请求
                auto req_msg = only_days::MessageFactory::create<RpcRequest>();
                //auto req_msg = only_days::MessageFactory::create(only_days::MType::REQ_RPC)
                req_msg->setId(UUID::uuid());
                req_msg->setMType(MType::REQ_RPC);
                req_msg->setMethod(method);
                req_msg->setParams(params);

                //2.发送请求
                //为什么需要std::dynamic_pointer_cast<BaseMessage>(req_msg)这行代码？
                //因为继承是允许父类指针指向子类指针的，但是这种情况再函数重载时不允许！！！
                //因为重载函数时，会要对函数的参数进行分辨查找
                //std::dynamic_pointer_cast是智能指针安全向下转型的工具
                BaseMessage::ptr rsp_msg;
                bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), rsp_msg);
                if (ret == false)
                {
                    ELOG("同步Rpc请求失败！");
                    return false;
                }
                DLOG("收到响应，进行解析，获取结果!");

                //3.等待响应
                auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(rsp_msg);
                if (!rpc_rsp_msg)
                {
                    ELOG("rpc响应，向下类型转换失败！");
                    return false;
                }
                if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                {
                    ELOG("rpc请求出错：%s", errReason(rpc_rsp_msg->rcode()).c_str());
                    return false;
                }
                result = rpc_rsp_msg->result();
                DLOG("结果设置完毕！");
                return true;
            }

            //异步
            bool call(const BaseConnection::ptr &conn, const std::string &method, const Json::Value &params, JsonAsyncResponse &result)
            {
                auto req_msg = MessageFactory::create<RpcRequest>();
                req_msg->setId(UUID::uuid());
                req_msg->setMType(MType::REQ_RPC);
                req_msg->setMethod(method);
                req_msg->setParams(params);
                
                //为什么要使用异步回调的send，而不是直接用异步的send？
                //向服务器发送异步回调请求，设置回调函数，回调函数中会传入一个promise对象，在回调函数中对promise设置数据

                //为什么要使用promise的智能指针，因为一但出了这个函数
                //该promise就会失效，而之前绑定的result就会找不到对应的promise，从而报异常
                //这里要注意的是，当传参给回调函数时，直接传智能指针，而非智能指针的引用
                //这样就能保证该智能指针的计数器会加一，此时的计数器为2
                //即使后面退出该函数，该智能指针的计数器减一后还是1，已经能用来接收promise中的Json::Value的内容
                auto json_promise = std::make_shared<std::promise<Json::Value>>();
                result = json_promise->get_future();
                Requestor::RequestCallback cb = std::bind(&RpcCaller::Callback, this, json_promise, std::placeholders::_1);
                /*
                 * 【回调函数适配原理】
                 * 当业务函数（如Callback）参数多于回调接口（如RequestCallback单参数）时
                 * 用std::bind/lambda将“固定参数”（如this、promise）提前绑死，仅留“动态参数”（如消息msg）用占位符（_1）表示。
                 * 调用时网络层只传动态参数（msg），适配器自动将“固定参数+动态参数”组合后传给原函数，实现多参函数适配单参接口，解耦网络与业务逻辑。
                 */
                bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), cb);
                if (ret == false)
                {
                    ELOG("异步Rpc请求失败！");
                    return false;
                }
                return true;
            }

            //回调
            bool call(const BaseConnection::ptr &conn, const std::string &method, const Json::Value &params, const JsonResponseCallback &cb)
            {
                auto req_msg = MessageFactory::create<RpcRequest>();
                req_msg->setId(UUID::uuid());
                req_msg->setMType(MType::REQ_RPC);
                req_msg->setMethod(method);
                req_msg->setParams(params);

                Requestor::RequestCallback req_cb = std::bind(&RpcCaller::Callback1, this, cb, std::placeholders::_1);
                bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), req_cb);
                if (ret == false)
                {
                    ELOG("回调Rpc请求失败！");
                    return false;
                }
                return true;
            }

        private:
            void Callback1(const JsonResponseCallback &cb, const BaseMessage::ptr &msg)
            {
                auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                if (!rpc_rsp_msg)
                {
                    ELOG("rpc响应，向下类型转换失败！");
                    return;
                }
                if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                {
                    ELOG("rpc回调请求出错：%s", errReason(rpc_rsp_msg->rcode()).c_str());
                    return;
                }
                cb(rpc_rsp_msg->result());
            }

            void Callback(std::shared_ptr<std::promise<Json::Value>> result, const BaseMessage::ptr &msg)
            {
                auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                if (!rpc_rsp_msg)
                {
                    ELOG("rpc响应，向下类型转换失败！");
                    return;
                }
                if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                {
                    ELOG("rpc异步请求出错：%s", errReason(rpc_rsp_msg->rcode()).c_str());
                    return;
                }
                result->set_value(rpc_rsp_msg->result());
            }
        private:
            Requestor::ptr _requestor;
        };
    }
}