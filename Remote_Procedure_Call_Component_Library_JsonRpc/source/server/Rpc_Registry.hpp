#pragma once

#include "../common/Net.hpp"
#include "../common/Message.hpp"

#include<unordered_map> 
#include<set>


namespace only_days
{
    namespace server
    {
        class ProviderManager
        {
            public:
                using ptr = std::shared_ptr<ProviderManager>;

                struct Provider
                {
                    using ptr = std::shared_ptr<Provider>;

                    Provider(const BaseConnection::ptr& conn,const Address& host)
                    :_conn(conn)
                    ,_host(host)
                    {}
                    //1.主机地址(外部访问地址)
                    // Message.hpp里有typedef std::pair<std::string, int> Address;
                    Address _host;
                    //2.提供的服务名称
                    std::vector<std::string> _methods;
                    //3.连接
                    BaseConnection::ptr _conn;
                    //4.加锁，保护数据的增删操作
                    std::mutex _mutex;

                    void appendMethod(const std::string& method)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        //empalce_back
                        _methods.emplace_back(method);
                    }
                };
                //当一个新的服务提供者进行服务注册时调用
                Provider::ptr addProvider(const BaseConnection::ptr& conn,const Address& host,const std::string& method)
                {
                    //查找连接所关联的服务提供对象，找到则获取，找不到则创建，并建立关联
                    Provider::ptr provider;
                    //限制加锁的周期
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(conn);
                        if(it != _conns.end())
                        {
                            provider = it->second;
                        }else{
                            provider = std::make_shared<Provider>(conn, host);
                            _conns.insert(make_pair(conn,provider));
                        }
                        //method方法的提供主机要多出一个，_providers新增数据
                        //如果method存在，则会返回该method的提供者的vector，反之则返回一个空的vector
                        auto &providers = _providers[method];
                        providers.insert(provider);
                        return provider;
                    }
                    //向服务对象中新增一个所能提供的服务名称
                    provider->appendMethod(method);
                }
                //当一个服务提供者断开连接时，获取他的信息 --- 用于进行服务下线通知
                Provider::ptr getProvider(const BaseConnection::ptr& conn)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(conn);
                        if(it != _conns.end())
                        {
                            return it->second;
                        }
                        return Provider::ptr();
                }
                //当一个服务提供者断开连接的时候，删除他的关联信息
                void deleteProvider(const BaseConnection::ptr& conn)
                {
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(conn);
                        if(it == _conns.end())
                        {
                            //当前断开连接的不是一个服务提供者
                            return ;
                        }
                        //如果是提供者，看看提供了什么服务，从服务提供者信息中删除当前服务提供者
                        for(auto & method : it->second->_methods)
                        {
                            auto &providers = _providers[method];
                            providers.erase(it->second);
                        }
                        //删除链接与服务提供者的关联关系
                        _conns.erase(it);
                    }
                }
                std::vector<Address> methodHosts(const std::string &method)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _providers.find(method);
                    if(it == _providers.end())
                    {
                        return std::vector<Address>();
                    }
                    std::vector<Address> result;
                    for (auto &p : it->second)
                    {
                        result.push_back(p->_host);
                    }
                    return result;
                }
            private:
                std::mutex _mutex;
                std::unordered_map<std::string,std::set<Provider::ptr>> _providers;
                std::unordered_map<BaseConnection::ptr,Provider::ptr> _conns;
        };

        class DiscovererManager
        {
            public:
                using ptr = std::shared_ptr<DiscovererManager>;

                struct Discoverer
                {
                    using ptr = std::shared_ptr<Discoverer>;

                    Discoverer(const BaseConnection::ptr& conn)
                    :_conn(conn)
                    {}

                    //1.发现者关联的客户端连接
                    BaseConnection::ptr _conn;
                    //2.发现过的服务名称
                    std::vector<std::string> _methods;
                    //3.加锁，保护数据的增删操作
                    std::mutex _mutex;
                    void appendMethod(const std::string& method)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        //empalce_back
                        _methods.emplace_back(method);
                    }
                };

                //当每次客户端进行服务发现的时候新增发现者，新增服务名称
                Discoverer::ptr addDiscoverer(const BaseConnection::ptr& conn, const std::string& method)
                {
                    Discoverer::ptr discoverer;
                    //限制加锁的周期
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(conn);
                        if(it != _conns.end())
                        {
                            discoverer = it->second;
                        }else{
                            discoverer = std::make_shared<Discoverer>(conn);
                            _conns.insert(make_pair(conn,discoverer));
                        }
                        //一定要用引用
                        auto &discoverers = _discoverers[method];
                        discoverers.insert(discoverer);
                    }

                    discoverer->appendMethod(method);
                    return discoverer;
                }
                //发现者客户端断开连接时，找到发现者信息，删除关联数据
                void deleteDiscoverer(const BaseConnection::ptr& conn)
                {
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(conn);
                        if(it == _conns.end())
                        {
                            //没有找到连接对应的发现者消息，代表客户端不是一个服务发现者
                            return ;
                        }
                        for(auto & method : it->second->_methods)
                        {
                            auto &discoverers = _discoverers[method];
                            discoverers.erase(it->second);
                        }
                        //删除链接与服务提供者的关联关系
                        _conns.erase(it);
                    }
                }
                //当有一个新的服务提供者上线，则进行上线通知
                void onlineNotify(const std::string &method, const Address& host)
                {
                    notify(method,host,only_days::ServiceOptype::SERVICE_ONLINE);
                }
                //当有一个服务提供者断开连接，则进行下线通知
                void offlineNotify(const std::string &method, const Address& host)
                {
                    notify(method,host,only_days::ServiceOptype::SERVICE_OFFLINE);
                    
                }
            private:
                void notify(const std::string &method, const Address& host, only_days::ServiceOptype optype)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _discoverers.find(method);
                    if(it == _discoverers.end())
                    {
                        //这代表这个服务当前没有发现者
                        return;
                    }
                    auto msg_req = MessageFactory::create<ServiceRequest>();
                    msg_req->setId(UUID::uuid());
                    msg_req->setMethod(method);
                    msg_req->setMType(only_days::MType::REQ_SERVICE);
                    msg_req->setServiceHost(host);
                    msg_req->setServiceOptype(optype);

                    for(auto &discoverer : it->second)
                    {
                        discoverer->_conn->send(msg_req);
                    }
                }

            private:
                std::mutex _mutex;
                std::unordered_map<std::string,std::set<Discoverer::ptr>> _discoverers;
                std::unordered_map<BaseConnection::ptr,Discoverer::ptr> _conns;
        };

        class Provider_Discoverer_Manager
        {
            public:
                using ptr = std::shared_ptr<Provider_Discoverer_Manager>;
                Provider_Discoverer_Manager()
                :_providers(std::make_shared<ProviderManager>())
                ,_discoverers(std::make_shared<DiscovererManager>())
                {}
                void onServiceRequest(const BaseConnection::ptr& conn,const ServiceRequest::ptr &msg)
                {
                    //服务操作请求：服务注册/服务发现
                    ServiceOptype optype = msg->serviceOptype();
                    if(optype == only_days::ServiceOptype::SERVICE_REGISTRY)
                    {
                        //服务注册：
                        //  1.新增服务提供者；2.进行服务上线的通知
                        DLOG("%s:%d注册服务 %s",msg->serviceHost().first.c_str(),msg->serviceHost().second,msg->method().c_str());
                        _providers->addProvider(conn,msg->serviceHost(),msg->method());
                        _discoverers->onlineNotify(msg->method(),msg->serviceHost());
                        return registryResponse(conn, msg);
                    }
                    else if(optype == only_days::ServiceOptype::SERVICE_DISCOVERY)
                    {
                        //服务发现：
                        //  1.新增服务发现者
                        DLOG("客户端要进行%s服务发现！",msg->method().c_str());
                        _discoverers->addDiscoverer(conn,msg->method());
                        return discoveryResponse(conn, msg);
                    }else{
                        DLOG("收到服务请求，但是操作类型错误!");
                        return errorResponse(conn, msg);
                    }
                }
                void onShutdown(const BaseConnection::ptr& conn)
                {
                    auto provider = _providers->getProvider(conn);
                    if(provider.get() != nullptr)
                    {
                        DLOG("%s:%d注册下线",provider->_host.first.c_str(),provider->_host.second);
                        for(auto &method : provider->_methods)
                        {
                            _discoverers->offlineNotify(method,provider->_host);
                        }
                        _providers->deleteProvider(conn);
                    }
                    _discoverers->deleteDiscoverer(conn);
                }
            private:
                void errorResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr &msg)
                {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setId(msg->id());
                    msg_rsp->setMType(MType::RSP_SERVICE);
                    msg_rsp->setRcode(RCode::RCODE_INVALID_OPTYPE);
                    msg_rsp->setServiceOptype(ServiceOptype::SERVICE_UNKNOW);
                    conn->send(msg_rsp);
                }

                void registryResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr &msg)
                {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setId(msg->id());
                    msg_rsp->setMType(MType::RSP_SERVICE);
                    msg_rsp->setRcode(RCode::RCODE_OK);
                    msg_rsp->setServiceOptype(ServiceOptype::SERVICE_REGISTRY);
                    conn->send(msg_rsp);
                }

                void discoveryResponse(const BaseConnection::ptr& conn,const ServiceRequest::ptr &msg)
                {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    std::vector<Address> hosts = _providers->methodHosts(msg->method());
                    //必须具备的元素
                    msg_rsp->setId(msg->id());
                    msg_rsp->setMType(MType::RSP_SERVICE);
                    msg_rsp->setServiceOptype(ServiceOptype::SERVICE_DISCOVERY);
                    if(hosts.empty())
                    {
                        msg_rsp->setRcode(RCode::RCODE_NOT_FOUND_SERVICE);
                        return conn->send(msg_rsp);
                    }
                    
                    msg_rsp->setRcode(RCode::RCODE_OK);
                    msg_rsp->setMethod(msg->method());
                    msg_rsp->setServiceHost(hosts);
                    conn->send(msg_rsp);
                }
            private:
                ProviderManager::ptr _providers;
                DiscovererManager::ptr _discoverers;
        };
    }
}