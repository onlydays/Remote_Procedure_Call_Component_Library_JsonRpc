#pragma once
#include "../common/Net.hpp"
#include "../common/Message.hpp"
#include <future>
#include <functional>



//我的问题，为什么要设置异步与同步，我还是有点搞不清为什么要这样设置？
namespace only_days
{
    namespace client
    {
        class Requestor
        {
        public:
            using ptr = std::shared_ptr<Requestor>; 
            using RequestCallback = std::function<void(const BaseMessage::ptr &)>;
            using AsyncResponse = std::future<BaseMessage::ptr>;

            struct RequestDescribe
            {
                using ptr = std::shared_ptr<RequestDescribe>;
                BaseMessage::ptr request;
                RType rtype;
                std::promise<BaseMessage::ptr> response;
                RequestCallback callback;
            };

            void onResponse(const BaseConnection::ptr &conn, BaseMessage::ptr &msg)
            {
                std::string rid = msg->id();
                RequestDescribe::ptr rdp = getDescribe(rid);
                if (rdp.get() == nullptr)
                {
                    ELOG("收到响应 - %s，但是未找到对应的请求描述！", rid.c_str());
                    return;
                }
                if (rdp->rtype == RType::REQ_ASYNC)
                {
                    rdp->response.set_value(msg);
                }
                else if (rdp->rtype == RType::REQ_CALLBACK)
                {
                    if (rdp->callback)
                        rdp->callback(msg);
                }
                else
                {
                    ELOG("请求类型未知！！");
                }
                delDescribe(rid);
            }

            //异步:将async_rsp和要发送的信息的response进行绑定
            //当响应来了之后信息就会被设置进response
            //然后外界就可以通过与response关联的async_rsp来得到响应的内容
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, AsyncResponse &async_rsp)
            {
                RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_ASYNC);
                if (rdp.get() == nullptr)
                {
                    ELOG("构造请求描述对象失败！");
                    return false;
                }
                conn->send(req);
                async_rsp = rdp->response.get_future();
                return true;
            }

            //同步:将异步后的结果从async_rsp中拿到，然后直接get获取
            //但是由于使用了get，那么知道有响应之前，该程序将会一直阻塞在这里！！！
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, BaseMessage::ptr &rsp)
            {
                AsyncResponse rsp_future;
                bool ret = send(conn, req, rsp_future);
                if (ret == false)
                {
                    return false;
                }
                rsp = rsp_future.get();
                return true;
            }

            //异步回调
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, const RequestCallback &cb)
            {
                RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_CALLBACK, cb);
                if (rdp.get() == nullptr)
                {
                    ELOG("构造请求描述对象失败！");
                    return false;
                }
                conn->send(req);
                return true;
            }

        private:
            RequestDescribe::ptr newDescribe(const BaseMessage::ptr &req, RType rtype, const RequestCallback &cb = RequestCallback())
            {
                std::unique_lock<std::mutex> lock(_mutex);
                RequestDescribe::ptr rd = std::make_shared<RequestDescribe>();
                rd->request = req;
                rd->rtype = rtype;
                if (rtype == RType::REQ_CALLBACK && cb)
                {
                    rd->callback = cb;
                }
                _request_desc.insert(std::make_pair(req->id(), rd));
                return rd;
            }

            RequestDescribe::ptr getDescribe(const std::string &rid)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _request_desc.find(rid);
                if (it == _request_desc.end())
                {
                    return RequestDescribe::ptr();
                }
                return it->second;
            }

            void delDescribe(const std::string &rid)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _request_desc.erase(rid);
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, RequestDescribe::ptr> _request_desc;
        };

    }
}