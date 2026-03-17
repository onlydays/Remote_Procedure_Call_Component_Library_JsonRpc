#pragma once

#include<memory>
#include<functional>
#include"Fields.hpp"

namespace only_days{
    class BaseMessage{
        public:
            //typedef std::shared_ptr<BaseMessage> ptr;  // C++98风格
            using ptr = std::shared_ptr<BaseMessage>;
            virtual ~BaseMessage(){}
            //设置消息类型
            virtual void setMType(MType mtype){
                _mtype = mtype;
            }
            //设置消息id
            virtual void setId(const std::string &id){
                _rid = id;
            }
            //返回消息类型
            virtual MType mtype(){  return _mtype; }
            //返回消息id
            virtual std::string id(){   return _rid; }
            //对消息进行序列化
            virtual std::string serialize() = 0;
            //对消息进行反序列化
            virtual bool unserialize(const std::string &msg) = 0;
            //检查消息是否合法或完整
            virtual bool check() = 0;
        private:
           MType _mtype;
           std::string _rid; 
    };

    class BaseBuffer
    {
    public:
        //typedef std::shared_ptr<BaseBuffer> ptr;  // C++98风格
        using ptr = std::shared_ptr<BaseBuffer>;
        //缓冲区中有多少数据
        virtual size_t readableSize() = 0;
        //尝试从缓冲区中取出四字节数据，也就是查看缓冲区中有没有4字节数据
        virtual int32_t peekInt32() = 0;
        //从缓冲区中删去四个字节数据
        virtual void retrieveInt32() = 0;
        //直接从缓冲区中取出四字节数据，并删除缓冲区当中的这四个字节
        virtual int32_t readInt32() = 0;
        //根据长度来取出缓冲区当中的指定数据,然后删除对应长度的数据
        virtual std::string retrieveAsString(size_t len) = 0;
    };

    class BaseProtocol
    {
    public:
        using ptr = std::shared_ptr<BaseProtocol>;
        //该buffer是否能够处理为message
        virtual bool canProcessed(const BaseBuffer::ptr &buf) = 0;
        //将buffer处理成消息message
        virtual bool onMessage(const BaseBuffer::ptr &buf, BaseMessage::ptr &msg) = 0;
        //将消息进行序列化 
        virtual std::string serialize(const BaseMessage::ptr &msg) = 0;
    };

    class BaseConnection
    {
    public:
        using ptr = std::shared_ptr<BaseConnection>;
        //发送数据
        virtual void send(const BaseMessage::ptr &msg) = 0;
        //为什么没有recv函数呢？
        //这是因为在当前服务器都是异步操作
        //这种异步操作都是通过缓冲区的事件触发
        //是通过事件触发来进行操作而不是上层直接去recv获得数据
        //是得到数据之后，数据在缓冲区中，触发回调函数进行处理
        //关闭连接
        virtual void shutdown() = 0;
        //建立连接
        virtual bool connected() = 0;
    };

    //提前写出三种回调函数，有了回调函数之后，通过BaseProtocol获取缓冲区中的数据
    //并进行处理，处理完之后再调用回调函数
    //连接建立时的回调函数
    using ConnectionCallBack = std::function<void(const BaseConnection::ptr&)>;
    //关闭连接时的回调函数
    using CloseCallBack = std::function<void(const BaseConnection::ptr&)>;
    //消息的处理的回调函数
    using MessageCallBack = std::function<void(const BaseConnection::ptr&,BaseMessage::ptr&)>;
    class BaseServer
    {
    public:
        using ptr = std::shared_ptr<BaseServer>;
        virtual void setConnectionCallback(const ConnectionCallBack &cb){
            _cb_connection = cb;
        }
        virtual void setCloseCallback(const CloseCallBack &cb){
            _cb_close = cb;
        }
        virtual void setMessageCallback(const MessageCallBack &cb){
            _cb_message = cb;
        }
        virtual void start() = 0;
    protected:
        ConnectionCallBack _cb_connection;
        CloseCallBack _cb_close;
        MessageCallBack _cb_message;
    };

    class BaseClient
    {
    public:
        using ptr = std::shared_ptr<BaseClient>;
        virtual void setConnectionCallback(const ConnectionCallBack &cb){
            _cb_connection = cb;
        }
        virtual void setCloseCallback(const CloseCallBack &cb){
            _cb_close = cb;
        }
        virtual void setMessageCallback(const MessageCallBack &cb){
            _cb_message = cb;
        }
        //连接服务器
        virtual void connect() = 0;
        //关闭连接
        virtual void shutdown() = 0;
        //发送消息
        virtual bool send(const BaseMessage::ptr &) = 0;
        //向外提供一个获取连接对象
        virtual BaseConnection::ptr connection() = 0;
        //判断连接是否正常
        virtual bool connected() = 0;
    protected:
        ConnectionCallBack _cb_connection;
        CloseCallBack _cb_close;
        MessageCallBack _cb_message;
    };
}