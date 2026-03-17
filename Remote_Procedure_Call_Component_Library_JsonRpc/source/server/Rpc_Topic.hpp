#pragma once
#include "../common/Net.hpp"
#include "../common/Message.hpp"

#include<unordered_set>

namespace only_days
{
    namespace server
    {
        class TopicManager
        {
            public:
                using ptr = std::shared_ptr<TopicManager>;

                void onTopicRequest(const BaseConnection::ptr& conn,const TopicRequest::ptr& msg)
                {
                    TopicOptype topic_optype = msg->topicOptype();
                    bool ret = true;
                    switch(topic_optype)
                    {
                        //主题的创建
                        case TopicOptype::TOPIC_CREATE: topicCreate(conn,msg);break;
                        //主题的删除
                        case TopicOptype::TOPIC_REMOVE: topicRemove(conn,msg);break;
                        //主题的订阅
                        case TopicOptype::TOPIC_SUBSCRIBE: ret = topicSubscribe(conn,msg);break;
                        //主题的取消订阅
                        case TopicOptype::TOPIC_CANCEL: topicCancel(conn,msg);break;
                        //主题消息的发送
                        case TopicOptype::TOPIC_PUBLISH: ret = topicPublish(conn,msg);break;
                        default: return errorResponse(conn,msg,RCode::RCODE_INVALID_OPTYPE);
                    }
                    if(!ret) return errorResponse(conn,msg,RCode::RCODE_NOT_FOUND_TOPIC);
                    return topicResponse(conn,msg);
                }
                //一个订阅者在连接断开时的处理 --- 删除其管理的数据
                void onShutDown(const BaseConnection::ptr& conn)
                {
                    //消息发布者断开连接，不需要任何操作
                    //消息订阅者断开连接需要删除管理数据
                    //1.判断断开连接的是否是订阅者，如果不是则直接返回
                    std::vector<Topic::ptr> topics;
                    Subcriber::ptr subscriber;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _subcribers.find(conn);
                        if(it == _subcribers.end())
                        {
                            return ;//断开连接，不是一个订阅者的连接
                        }
                        subscriber = it->second;
                        //2.获取到订阅者退出，受影响的主题对象
                        for(auto& topic_name : subscriber->_topic)
                        {
                            auto topic_it = _topics.find(topic_name);
                            if(topic_it == _topics.end()) continue;
                            topics.push_back(topic_it->second);
                        }
                        //4.从订阅者映射信息中，删除订阅者
                        _subcribers.erase(it);
                    }
                    //3.从主题对象中，移除订阅者
                    for(auto& topic : topics)
                    {
                        topic->removeSubsciber(subscriber);
                    }        
                }
            private:
                void errorResponse(const BaseConnection::ptr& conn,const TopicRequest::ptr &msg,const RCode& rcode)
                {
                    auto msg_rsp = MessageFactory::create<TopicResponse>();
                    msg_rsp->setId(msg->id());
                    msg_rsp->setMType(MType::RSP_TOPIC);
                    msg_rsp->setRcode(rcode);
                    conn->send(msg_rsp);
                }

                void topicResponse(const BaseConnection::ptr& conn,const TopicRequest::ptr &msg)
                {
                    auto msg_rsp = MessageFactory::create<TopicResponse>();
                    msg_rsp->setId(msg->id());
                    msg_rsp->setMType(MType::RSP_TOPIC);
                    msg_rsp->setRcode(RCode::RCODE_OK);
                    conn->send(msg_rsp);
                }
                //主题创建
                void topicCreate(const BaseConnection::ptr& conn,const TopicRequest::ptr& msg)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    //构造一个主题对象，添加映射关系的管理
                    std::string topic_name = msg->topicKey();
                    auto newtopic = std::make_shared<Topic>(topic_name);
                    _topics.insert(std::make_pair(topic_name, newtopic));
                }
                //主题删除
                void topicRemove(const BaseConnection::ptr& conn,const TopicRequest::ptr& msg)
                {
                    //1.查看当前主题有哪些订阅者，然后从订阅者中将主题信息删除掉
                    std::string topic_name = msg->topicKey();
                    Topic::ptr topic;
                    std::unordered_set<Subcriber::ptr> subcribers;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        //在删除主题之前，先找出回收影响的订阅者
                        auto it = _topics.find(topic_name);
                        if(it == _topics.end()) return ;
                        subcribers = it->second->_subscribers;
                        _topics.erase(it);
                    }
                    //2.删除主题的数据 --- 主题名称和主题对象的映射关系
                    for(auto& subcriber : subcribers)
                    {
                        subcriber->removeTopic(topic_name);
                    }
                }
                //主题订阅
                bool topicSubscribe(const BaseConnection::ptr& conn,const TopicRequest::ptr& msg)
                {
                    //1.先找出主题对象，以及订阅者对象
                    //  如果没有找到主题，就要报错，但是如果没有找到订阅者对象，那就要构造一个订阅者
                    std::string topic_name = msg->topicKey();
                    Topic::ptr topic;
                    Subcriber::ptr subcriber;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto topic_it = _topics.find(topic_name);
                        if(topic_it == _topics.end()) return false;

                        topic = topic_it->second;

                        auto sub_it = _subcribers.find(conn);
                        if (sub_it != _subcribers.end())
                        {
                            subcriber = sub_it->second;
                        }else
                        {
                            subcriber = std::make_shared<Subcriber>(conn);
                            _subcribers.insert(make_pair(conn,subcriber));
                        }
                    }

                    //2.在主题对象中，新增一个订阅者对象关联的连接，在订阅者对象中新增一个订阅主题
                    topic->appendSubsciber(subcriber);
                    subcriber->appendTopic(topic_name);
                    return true;
                }
                //取消订阅
                void topicCancel(const BaseConnection::ptr& conn,const TopicRequest::ptr& msg)
                {
                    //1.先找出主题对象，和订阅者对象
                    //  主题不存在就报错，订阅者不存在则返回（不需要报错）
                    std::string topic_name = msg->topicKey();
                    Topic::ptr topic;
                    Subcriber::ptr subcriber;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto topic_it = _topics.find(topic_name);
                        if(topic_it != _topics.end()) 
                        {
                            topic = topic_it->second;
                        }
                        auto sub_it = _subcribers.find(conn);
                        if (sub_it != _subcribers.end())
                        {
                            subcriber = sub_it->second;
                        }else
                        {
                            subcriber = std::make_shared<Subcriber>(conn);
                            _subcribers.insert(make_pair(conn,subcriber));
                        }
                    }
                    //2.从主题对象中删除当前的订阅者连接，从订阅者消息中删除所订阅的主题名称
                    if(subcriber) subcriber->removeTopic(topic_name);
                    if(topic && subcriber) topic->removeSubsciber(subcriber);
                }
                //消息发布
                bool topicPublish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
                {
                    std::string topic_name = msg->topicKey();
                    Topic::ptr topic;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto topic_it = _topics.find(topic_name);
                        if (topic_it == _topics.end())
                        {
                            return false;
                        }
                        topic = topic_it->second;
                    }
                    topic->pushMessage(msg);
                    return true;
                }

            private:
            struct Subcriber
                {
                    using ptr = std::shared_ptr<Subcriber>;
                    std::mutex _mutex;
                    BaseConnection::ptr _conn;
                    std::unordered_set<std::string> _topic;//订阅者所订阅的主题名称

                    Subcriber(const BaseConnection::ptr& conn)
                    :_conn(conn)
                    {}
                    //订阅主题的时候调用
                    void appendTopic(const std::string& topic_name)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _topic.insert(topic_name);
                    }
                    //主题被删除 或者 取消订阅的手机调用
                    void removeTopic(const std::string& topic_name)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _topic.erase(topic_name);
                    }
                };

                struct Topic
                {
                    using ptr = std::shared_ptr<Topic>;
                    std::mutex _mutex;
                    std::string _topic_name;
                    std::unordered_set<Subcriber::ptr> _subscribers;//当前主题的订阅者

                    Topic(const std::string& topic_name)
                    :_topic_name(topic_name)
                    {}
                    //新增订阅的时候调用
                    void appendSubsciber(const Subcriber::ptr& subcriber)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _subscribers.insert(subcriber);
                    }
                    //取消订阅或者订阅者连接断开的时候调用
                    void removeSubsciber(const Subcriber::ptr& subcriber)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _subscribers.erase(subcriber);
                    }
                    //收到消息发布请求的时候调用
                    void pushMessage(const BaseMessage::ptr& msg)
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        for(auto& subscriber: _subscribers)
                        {
                            // 新增判空：避免空指针访问
                            if (!subscriber || !subscriber->_conn)
                            {
                                continue;
                            }
                            subscriber->_conn->send(msg);
                        }
                    }
                };
            private:
                std::mutex _mutex;
                std::unordered_map<std::string,Topic::ptr> _topics;
                std::unordered_map<BaseConnection::ptr,Subcriber::ptr> _subcribers;
        };
    }
}