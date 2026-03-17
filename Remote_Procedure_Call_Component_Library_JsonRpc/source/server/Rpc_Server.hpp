#include "../common/Dispatcher.hpp"
#include "Rpc_Router.hpp"
#include "Rpc_Registry.hpp"
#include "Rpc_Topic.hpp"
#include "../client/Rpc_Client.hpp"

#include<unordered_map>
#include<vector>

namespace only_days
{
    namespace server
    {
        class RegistryServer
        {
            public:
                using ptr = std::shared_ptr<RegistryServer>;

                RegistryServer(int port)
                :_pd_manager(std::make_shared<Provider_Discoverer_Manager>())
                ,_dispatcher(std::make_shared<Dispatcher>())
                {
                    auto service_cb = std::bind(&Provider_Discoverer_Manager::onServiceRequest,_pd_manager.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE, service_cb);

                    _server = only_days::MuduoServerFactory::create(port);
                    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,_dispatcher.get(),\
                    std::placeholders::_1,std::placeholders::_2);
                    _server->setMessageCallback(message_cb);

                    auto close_cb = std::bind(&RegistryServer::onConnShutdown, this, std::placeholders::_1);
                    _server->setCloseCallback(close_cb);
                }
                void start()
                {
                    _server->start();
                }
            private:
                void onConnShutdown(const BaseConnection::ptr& conn)
                {
                    _pd_manager->onShutdown(conn);
                }
            private:
                Provider_Discoverer_Manager::ptr _pd_manager;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };

        class RpcServer
        {
            public:
                using ptr = std::shared_ptr<RpcServer>;

                //rpc_server端有两套地址信息
                //1. rpc服务提供端地址信息---必须是rpc服务器对外访问地址（云服务器---监听地址和访问地址不同）
                //2. 注册中心服务端地址信息---启用服务注册后，连接注册中心进行服务注册用的
                RpcServer(const Address& access_addr, bool enableRegistry = false, const Address& registry_server_addr = Address())
                :_access_addr(access_addr)
                ,_enableRegistry(enableRegistry)
                ,_router(std::make_shared<RpcRouter>())
                ,_dispatcher(std::make_shared<Dispatcher>())
                {
                    if(_enableRegistry)
                    {
                        _reg_client = std::make_shared<client::RegistryClient>(
                            registry_server_addr.first, registry_server_addr.second
                        );
                    }
                    //当前成员server是一个rpc的server，用于提供rpc服务
                    auto rpc_cb = std::bind(&only_days::server::RpcRouter::onRpcRequest,_router.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<only_days::RpcRequest>(only_days::MType::REQ_RPC,rpc_cb);

                    _server = only_days::MuduoServerFactory::create(access_addr.second);
                    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,_dispatcher.get(),\
                    std::placeholders::_1,std::placeholders::_2);
                    _server->setMessageCallback(message_cb);
                }
                
                void registerMethod(const ServiceDescribe::ptr &service)
                {
                    _router->registerMethod(service);
                    if(_enableRegistry)
                    {
                        _reg_client->registryMethod(service->method(), _access_addr);
                    }
                }
                void start()
                {
                    _server->start();
                }
            private:
                Address _access_addr;
                bool _enableRegistry;
                client::RegistryClient::ptr _reg_client;
                RpcRouter::ptr _router;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };

        class TopicServer
        {
            public:
                using ptr = std::shared_ptr<TopicServer>;

                TopicServer(int port)
                :_topic_manager(std::make_shared<TopicManager>())
                ,_dispatcher(std::make_shared<Dispatcher>())
                {
                    auto topic_cb = std::bind(&TopicManager::onTopicRequest,_topic_manager.get()
                        , std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC, topic_cb);

                    _server = only_days::MuduoServerFactory::create(port);
                    auto message_cb = std::bind(&only_days::Dispatcher::onMessage,_dispatcher.get(),\
                    std::placeholders::_1,std::placeholders::_2);
                    _server->setMessageCallback(message_cb);

                    auto close_cb = std::bind(&TopicServer::onConnShutdown, this, std::placeholders::_1);
                    _server->setCloseCallback(close_cb);
                }
                void start()
                {
                    _server->start();
                }
            private:
                void onConnShutdown(const BaseConnection::ptr& conn)
                {
                    _topic_manager->onShutDown(conn);
                }
            private:
                TopicManager::ptr _topic_manager;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };
    }
}