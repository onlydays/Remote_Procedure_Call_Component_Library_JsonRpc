#pragma once

#include "AbstractionLayer.hpp"
#include "Fields.hpp"
#include "Detail.hpp"
#include<vector>

namespace only_days
{
    typedef std::pair<std::string, int> Address;

    class JsonMessage : public BaseMessage
    {
    public:
        using ptr = std::shared_ptr<BaseMessage>;
        // 对消息进行序列化
        // override判断派生类对基类的虚函数进行重写是否符合规则，是一种强校验
        // 让我们的代码更加严谨
        virtual std::string serialize() override
        {
            std::string body;
            bool ret = JSON::serialize(_body, body);
            if (ret == false)
            {
                return std::string();
            }
            return body;
        }
        // 对消息进行反序列化
        virtual bool unserialize(const std::string &msg)
        {
            return JSON::unserialize(msg, _body);
        }
        // 检查消息是否合法或完整
        virtual bool check() = 0;
        // 为什么要变成protected，如果是private的话
        // 派生类继承基类之后就无法访问了
    protected:
        Json::Value _body;
    };

    class JsonRequest : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonRequest>;
    };

    class JsonResponse : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonResponse>;
        virtual bool check() override
        {
            // 在响应中，大部分的响应都只有响应状态码
            // 因此只需要判断响应码字段是否存在，类型是否正确
            if (_body[KEY_RCODE].isNull() == true)
            {
                ELOG("响应中没有响应状态码");
                return false;
            }
            // 判断该字段的类型是否是int类型
            if (_body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("响应状态码类型错误！");
                return false;
            }
            return true;
        }

        virtual RCode rcode()
        {
            return (RCode)_body[KEY_RCODE].asInt();
        }

        virtual void setRcode(RCode rcode)
        {
            _body[KEY_RCODE] = (int)rcode;
        }
    };

    // body中分为方法名称和参数
    class RpcRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<RpcRequest>;
        virtual bool check() override
        {
            // rpc请求中，包含请求方法名称（字符串），方法字段（对象）
            // 方法名称的类型应该是一个字符串
            if (_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
            {
                ELOG("RPC请求中没有方法名称或方法名称类型错误");
                return false;
            }
            // 参数的类型应该是一个JSON::Value的对象
            if (_body[KEY_PARAMS].isNull() == true ||
                _body[KEY_PARAMS].isObject() == false)
            {
                ELOG("RPC请求中没有参数信息或参数信息类型错误");
                return false;
            }
            return true;
        }
        // 返回方法的名称
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }
        // 设置方法名称
        void setMethod(const std::string &method)
        {
            _body[KEY_METHOD] = method;
        }
        // 返回参数的对象
        Json::Value params()
        {
            return _body[KEY_PARAMS];
        }
        // 设置参数对象
        void setParams(const Json::Value &params)
        {
            _body[KEY_PARAMS] = params;
        }
    };

    class TopicRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<TopicRequest>;
        virtual bool check() override
        {
            // 主题名称是一个字符串
            if (_body[KEY_TOPIC_KEY].isNull() == true ||
                _body[KEY_TOPIC_KEY].isString() == false)
            {
                ELOG("主题请求中没有主题名称或主题名称类型错误");
                return false;
            }
            // 操作类型是一个int类型
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("主题请求中没有操作类型或操作类型的类型错误");
                return false;
            }
            // 主题消息是一个字符串
            if (_body[KEY_OPTYPE].asInt() == (int)TopicOptype::TOPIC_PUBLISH &&
                (_body[KEY_TOPIC_MSG].isNull() == true || _body[KEY_TOPIC_MSG].isString() == false))
            {
                ELOG("主题消息发布请求中没有消息内容字段或消息内容类型错误");
                return false;
            }
            return true;
        }
        // 返回一个主题请求的主题名称
        std::string topicKey()
        {
            return _body[KEY_TOPIC_KEY].asString();
        }
        // 设置主题名称
        void setTopicKey(const std::string topic_key)
        {
            _body[KEY_TOPIC_KEY] = topic_key;
        }
        // 返回一个主题请求的操作类型
        TopicOptype topicOptype()
        {
            return (TopicOptype)_body[KEY_OPTYPE].asInt();
        }
        // 设置操作类型
        void setTopicOptype(TopicOptype optype)
        {
            _body[KEY_OPTYPE] = (int)optype;
        }
        // 返回一个主题请求的消息内容
        std::string topicMessage()
        {
            return _body[KEY_TOPIC_MSG].asString();
        }
        // 设置消息内容
        void setTopicMessage(const std::string topic_msg)
        {
            _body[KEY_TOPIC_MSG] = topic_msg;
        }
    };

    class ServiceRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<ServiceRequest>;
        virtual bool check() override
        {
            // 方法名称是一个字符串
            if (_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
            {
                ELOG("服务请求中没有方法名称或方法名称类型错误");
                return false;
            }
            // 操作类型是一个int类型
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("服务请求中没有操作类型或操作类型的类型错误");
                return false;
            }
            // 主机信息是一个Json::Value对象
            //服务发现的时候没有HOST字段!!!所以要特殊处理一下
            if (_body[KEY_OPTYPE].asInt() != (int)(ServiceOptype::SERVICE_DISCOVERY) &&
                (_body[KEY_HOST].isNull() == true ||
                _body[KEY_HOST].isObject() == false ||
                _body[KEY_HOST][KEY_HOST_IP].isNull() == true ||
                _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                _body[KEY_HOST][KEY_HOST_PORT].isNull() == true ||
                _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false))
            {
                ELOG("服务请求中，主机地址信息错误");
                return false;
            }
            return true;
        }
        // 返回方法的名称
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }
        // 设置方法名称
        void setMethod(const std::string &method)
        {
            _body[KEY_METHOD] = method;
        }
        // 返回一个主题请求的操作类型
        ServiceOptype serviceOptype()
        {
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }
        // 设置操作类型
        void setServiceOptype(ServiceOptype optype)
        {
            _body[KEY_OPTYPE] = (int)optype;
        }
        // 返回一个主机信息
        Address serviceHost()
        {
            Address addr;
            addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
            addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
            return addr;
        }
        // 设置主机信息
        void setServiceHost(const Address &addr)
        {
            Json::Value val;
            val[KEY_HOST_IP] = addr.first;
            val[KEY_HOST_PORT] = addr.second;
            _body[KEY_HOST] = val;
        }
    };

    class RpcResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<RpcResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true ||
                _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("响应中没有响应状态码,或状态码类型错误！");
                return false;
            }
            //取消对result的具体内容的约束，其中的内容可以是对象
            //也可以是int类型或者字符串等类型
            if (_body[KEY_RESULT].isNull() == true)
            {
                ELOG("响应中没有Rpc调⽤结果,或结果类型错误！");
                return false;
            }
            return true;
        }

        Json::Value result()
        {
            return _body[KEY_RESULT];
        }
        void setResult(const Json::Value &result)
        {
            _body[KEY_RESULT] = result;
        }
    };

    class TopicResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<TopicResponse>;
    };

    // service的请求里面包含了注册、上线、下线和发现这四个操作
    // 对于前三个操作，我们主需要进行一个常规的响应就可以了
    // 也就是只需返回操作是否成功的信息就行了（只需返回rcode）
    // 但是发现不一样，发现的操作需要返回的东西比较多
    // 如：rcode、method、host
    class ServiceResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<ServiceResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true ||
                _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("响应中没有响应状态码,或状态码类型错误！");
                return false;
            }
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("响应中没有操作类型,或操作类型的类型错误！");
                return false;
            }
            // 针对第四个操作进行处理
            if (_body[KEY_OPTYPE].asInt() == (int)(ServiceOptype::SERVICE_DISCOVERY) &&
                (_body[KEY_METHOD].isNull() == true ||
                 _body[KEY_METHOD].isString() == false ||
                 _body[KEY_HOST].isNull() == true ||
                 _body[KEY_HOST].isArray() == false))
            {
                ELOG("服务发现响应中响应信息字段错误！");
                return false;
            }
            return true;
        }
        ServiceOptype serviceOptype()
        {
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }
        void setServiceOptype(ServiceOptype optype)
        {
            _body[KEY_OPTYPE] = (int)optype;
        }
        // 针对第四个操作进行处理
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }
        void setMethod(const std::string &method)
        {
            _body[KEY_METHOD] = method;
        }
        void setServiceHost(std::vector<Address> addrs)
        {
            for (auto &addr : addrs)
            {
                Json::Value val;
                val[KEY_HOST_IP] = addr.first;
                val[KEY_HOST_PORT] = addr.second;
                _body[KEY_HOST].append(val);
            }
        }
        std::vector<Address> serviceHost()
        {
            std::vector<Address> addrs;
            int sz = _body[KEY_HOST].size();
            for (int i = 0; i < sz; i++)
            {
                Address addr;
                addr.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();
                addr.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();
                addrs.push_back(addr);
            }
            return addrs;
        }
    };

    //实现一个消息对象的生产工厂
    class MessageFactory{
        public:
            static BaseMessage::ptr create(MType mtype)
            {
                switch(mtype)
                {
                    case MType::REQ_RPC : return std::make_shared<RpcRequest>();
                    case MType::REQ_SERVICE : return std::make_shared<ServiceRequest>();
                    case MType::REQ_TOPIC : return std::make_shared<TopicRequest>();
                    case MType::RSP_RPC : return std::make_shared<RpcResponse>();
                    case MType::RSP_SERVICE : return std::make_shared<ServiceResponse>();
                    case MType::RSP_TOPIC : return std::make_shared<TopicResponse>();
                }
                return BaseMessage::ptr();
            }
            template <typename T, typename... Args>
            static std::shared_ptr<T> create(Args &&...args)
            {
                return std::make_shared<T>(std::forward(args)...);
            }
    };
}
