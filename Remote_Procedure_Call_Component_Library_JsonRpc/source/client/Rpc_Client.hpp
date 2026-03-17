#include "../common/Dispatcher.hpp"
#include "Requestor.hpp"
#include "RpcCaller.hpp"
#include "Rpc_Registry.hpp"
#include "Rpc_Topic.hpp"

#include<unordered_map>
#include<vector>

namespace only_days
{
    namespace client
    {
        class RegistryClient
        {
            public:
                using ptr = std::shared_ptr<RegistryClient>;
                //构造函数传入注册中心的地址信息，用于连接注册中心
                RegistryClient(const std::string& ip, int port)
                :_requestor(std::make_shared<Requestor>())
                ,_provider(std::make_shared<client::Provider>(_requestor))
                ,_dispatcher(std::make_shared<Dispatcher>())
                {
                    auto rsp_cb = std::bind(&only_days::client::Requestor::onResponse,_requestor.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<only_days::BaseMessage>(only_days::MType::RSP_SERVICE,rsp_cb);
                
                    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,_dispatcher.get(),\
                    std::placeholders::_1,std::placeholders::_2);
                    _client = only_days::MuduoClientFactory::create(ip,port);
                    _client->setMessageCallback(message_cb);
                    _client->connect();
                }
                //向外提供的服务注册接口
                bool registryMethod(const std::string& method, const Address& host)
                {
                    return _provider->registryMethod(_client->connection(), method, host);
                }
            private:
                Requestor::ptr _requestor;
                client::Provider::ptr _provider;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        class DiscoveryClient
        {
            public:
                using ptr = std::shared_ptr<DiscoveryClient>;
                //构造函数传入注册中心的地址信息，用于连接注册中心
                DiscoveryClient(const std::string& ip, int port, const Discoverer::OfflineCallback& offline_callback)
                :_requestor(std::make_shared<Requestor>())
                ,_discoverer(std::make_shared<client::Discoverer>(_requestor, offline_callback))
                ,_dispatcher(std::make_shared<Dispatcher>())
                {
                    auto rsp_cb = std::bind(&client::Requestor::onResponse,_requestor.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE,rsp_cb);

                    auto req_cb = std::bind(&client::Discoverer::onServiceRequest,_discoverer.get()
                        , std::placeholders::_1,std::placeholders::_2);
                    _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE,req_cb);
                
                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),\
                    std::placeholders::_1,std::placeholders::_2);
                    _client = only_days::MuduoClientFactory::create(ip, port);
                    _client->setMessageCallback(message_cb);
                    _client->connect();
                }
                //向外提供的服务发现接口
                bool serviceDiscovery(const std::string& method, Address& host)
                {
                    return _discoverer->serviceDiscovery(_client->connection(), method, host);
                }
                
            private:
                Requestor::ptr _requestor;
                client::Discoverer::ptr _discoverer;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        class RpcClient
        {
            public:
                using ptr = std::shared_ptr<RpcClient>;
                using JsonAsyncResponse = std::future<Json::Value>;
                using JsonResponseCallback = std::function<void(const Json::Value &)>;

                //enableDiscovery --- 表示是否启用服务发现功能
                //也决定了传入的地址信息是注册中心的地址，还是服务提供者的地址
                RpcClient(const std::string& ip, int port, bool enableDiscovery = false)
                :_enableDiscovery(enableDiscovery)
                ,_requestor(std::make_shared<Requestor>())
                ,_dispatcher(std::make_shared<Dispatcher>())
                ,_caller(std::make_shared<RpcCaller>(_requestor))
                {
                    //针对rpc请求后的响应进行的回调处理
                    auto rsp_cb = std::bind(&client::Requestor::onResponse,_requestor.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_RPC,rsp_cb);


                    //如果启用了服务器发现，地址信息是注册中心的地址，是服务发现客户端需要连接的地址
                    //则通过地址信息实例化_discovery_client
                    //如果没有启用服务器，则地址信息是服务提供者的地址，则直接实例化rpc_client
                    if(_enableDiscovery)
                    {
                        auto offline_client = std::bind(&RpcClient::delClient, this, std::placeholders::_1);
                        _discovery_client = std::make_shared<DiscoveryClient>(ip, port, offline_client);
                    }else
                    {
                        auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),\
                        std::placeholders::_1,std::placeholders::_2);
                        _rpc_client = only_days::MuduoClientFactory::create(ip, port);
                        _rpc_client->setMessageCallback(message_cb);
                        _rpc_client->connect();
                    }
                }

                //同步
                bool call(const std::string &method, const Json::Value &params, Json::Value &result)
                {
                    //获取服务提供者：
                    //1.服务发现
                    //2.固定服务提供者
                    BaseClient::ptr client = getClient(method);
                    if(client.get() == nullptr) return false;
                        
                    //3.通过客户端连接，发送rpc请求
                    return _caller->call(client->connection(), method, params, result);
                }

                //异步
                bool call(const std::string &method, const Json::Value &params, JsonAsyncResponse &result)
                {
                    BaseClient::ptr client = getClient(method);
                    if(client.get() == nullptr) return false;
                        
                    //3.通过客户端连接，发送rpc请求
                    return _caller->call(client->connection(), method, params, result);
                }
                
                //异步回调
                bool call(const std::string &method, const Json::Value &params, const JsonResponseCallback &cb)
                {
                    BaseClient::ptr client = getClient(method);
                    if(client.get() == nullptr) return false;
                        
                    //3.通过客户端连接，发送rpc请求
                    return _caller->call(client->connection(), method, params, cb);
                }
            private:
                BaseClient::ptr newClient(const Address& host)
                {
                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),\
                    std::placeholders::_1,std::placeholders::_2);
                    auto client = only_days::MuduoClientFactory::create(host.first, host.second);
                    client->setMessageCallback(message_cb);
                    client->connect();
                    
                    putClient(host,client);
                    return client;
                }
                BaseClient::ptr getClient(const Address& host)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _rpc_clients.find(host);
                    if(it == _rpc_clients.end())
                    {
                        return BaseClient::ptr();
                    }
                    return it->second;
                }
                BaseClient::ptr getClient(const std::string& method)
                {
                    BaseClient::ptr client;
                    if(_enableDiscovery)
                    {
                        //1.通过服务发现，获取服务提供者地址信息
                        Address host;
                        bool ret = _discovery_client->serviceDiscovery(method,host);
                        if(ret == false)
                        {
                            DLOG("当前 %s 服务，没有找到服务提供者！",method.c_str());
                            return BaseClient::ptr();
                        }
                        //2.查看服务提供者是否已有实例化客户端，有则直接使用，没有则创建
                        client = getClient(host);
                        if(client.get() == nullptr)
                        {
                            //没有找到已经实例化的客户端，则要进行创建
                            client = newClient(host);
                        }
                    }else
                    {
                        client = _rpc_client;
                    }
                    return client;
                }
                void putClient(const Address& host,BaseClient::ptr& client)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _rpc_clients.insert(make_pair(host,client));
                }
                void delClient(const Address& host)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _rpc_clients.erase(host);
                }
            private:
                struct AddressHash{
                    size_t operator()(const Address& host) const
                    {
                        std::string addr = host.first + std::to_string(host.second);
                        return std::hash<std::string>{}(addr);
                    }
                };
            private:
                bool _enableDiscovery;
                Requestor::ptr _requestor;
                Dispatcher::ptr _dispatcher;
                RpcCaller::ptr _caller;
                DiscoveryClient::ptr _discovery_client;
                BaseClient::ptr _rpc_client;//用于未启用服务发现
                std::mutex _mutex;
                //<{127.0.0.1, 9090},[client1,client2]>;
                //采用长连接
                std::unordered_map<Address, BaseClient::ptr, AddressHash>_rpc_clients;//用于服务发现的客户端连接池
        };

        class TopicClient
        {
            public:
                using ptr = std::shared_ptr<TopicClient>;

                TopicClient(const std::string& ip, int port)
                :_requestor(std::make_shared<Requestor>())
                ,_dispatcher(std::make_shared<Dispatcher>())
                ,_topic_manager(std::make_shared<TopicManager>(_requestor))
                {
                    auto rsp_cb = std::bind(&Requestor::onResponse,_requestor.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_TOPIC,rsp_cb);
                    
                    auto msg_cb = std::bind(&TopicManager::onPublish,_topic_manager.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC,msg_cb);

                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),\
                        std::placeholders::_1,std::placeholders::_2);
                    _rpc_client = only_days::MuduoClientFactory::create(ip, port);
                    _rpc_client->setMessageCallback(message_cb);
                    _rpc_client->connect();
                }

                bool create(const std::string& key)
                {
                    return _topic_manager->create(_rpc_client->connection(), key);
                }
                bool remove(const std::string& key)
                {
                    return _topic_manager->remove(_rpc_client->connection(), key);
                }
                bool subscribe(const std::string& key,const TopicManager::SubCallback& cb)
                {
                    return _topic_manager->subscribe(_rpc_client->connection(), key, cb);
                }
                bool cancel(const std::string& key)
                {
                    return _topic_manager->cancel(_rpc_client->connection(), key);
                }
                bool publish(std::string& key, const std::string& msg)
                {
                    return _topic_manager->publish(_rpc_client->connection(), key, msg);
                }
                void shutdown()
                {
                    _rpc_client->shutdown();
                }
            private:
                Requestor::ptr _requestor;
                TopicManager::ptr _topic_manager;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _rpc_client;
        };
    }
}