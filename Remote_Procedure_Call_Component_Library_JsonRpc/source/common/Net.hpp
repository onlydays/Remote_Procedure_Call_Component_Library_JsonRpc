#pragma once
#include <iostream>
#include <string>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/EventLoopThread.h>

#include <unordered_map>
#include <mutex>

#include "AbstractionLayer.hpp"
#include "Fields.hpp"
#include "Detail.hpp"
#include "Message.hpp"

using std::cout;
using std::endl;

namespace only_days
{
    class MuduoBuffer : public BaseBuffer
    {
    public:
        using ptr = std::shared_ptr<MuduoBuffer>;
        MuduoBuffer(muduo::net::Buffer *buf)
            : _buf(buf)
        {
        }
        virtual size_t readableSize() override
        {
            return _buf->readableBytes();
        }
        virtual int32_t peekInt32() override
        {
            // muduo库是⼀个⽹络库，从缓冲区取出⼀个4字节整形，会进⾏⽹络字节序的转换
            //将数据从网络字节序列转换为主机字节序列
            return _buf->peekInt32();
        }
        virtual void retrieveInt32() override
        {
            return _buf->retrieveInt32();
        }
        virtual int32_t readInt32() override
        {
            return _buf->readInt32();
        }
        virtual std::string retrieveAsString(size_t len) override
        {
            return _buf->retrieveAsString(len);
        }

    private:
        // 这个指针不能随意释放，所以不使用智能指针
        muduo::net::Buffer *_buf;
    };
    class BufferFactory
    {
    public:
        template <typename... Args>
        static BaseBuffer::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...);
        }
    };

    //形式为8-4-4-4-12的32个字符
    //如：550e8400-e29b-41d4-a716-446655440000。(16个字节)
    class LVProtocol : public BaseProtocol
    {
    public:
        // |--Len--|--VALUE--|
        // |--Len--|--mtype--|--idlen--|--id--|--body--|
        using ptr = std::shared_ptr<LVProtocol>;
        // 判断缓冲区中的数据量是否⾜够⼀条消息的处理
        virtual bool canProcessed(const BaseBuffer::ptr &buf) override
        {
            //判断缓冲区中的数据有多少，是否大于四字节
            if (buf->readableSize() < lenFieldsLength)
            {
                return false;
            }
            //取出的四字节里面就包含整个消息的长度(不包括total_len自己)
            int32_t total_len = buf->peekInt32();
            // DLOG("total_len:%d", total_len);
            if (buf->readableSize() < (total_len + lenFieldsLength))
            {
                return false;
            }
            return true;
        }
        // |--Len--|--mtype--|--idlen--|--id--|--body--|
        //4-4-4-16-x
        virtual bool onMessage(const BaseBuffer::ptr &buf,BaseMessage::ptr &msg) override
        {
            // 当调⽤onMessage的时候，默认认为缓冲区中的数据⾜够⼀条完整的消息
            int32_t total_len = buf->readInt32();  // 读取总⻓度
            MType mtype = (MType)buf->readInt32(); // 读取数据类型
            int32_t idlen = buf->readInt32();      // 读取id⻓度
            int32_t body_len = total_len - idlen - idlenFieldsLength - mtypeFieldsLength;
            std::string id = buf->retrieveAsString(idlen);
            std::string body = buf->retrieveAsString(body_len);
            //返回一个shared_ptr指向的消息类型
            msg = only_days::MessageFactory::create(mtype);
            //get函数为shared_ptr自带的成员函数。
            if (msg.get() == nullptr)
            {
                ELOG("消息类型错误，构造消息对象失败！");
                return false;
            }
            bool ret = msg->unserialize(body);
            if (ret == false)
            {
                ELOG("消息正⽂反序列化失败！");
                return false;
            }
            msg->setId(id);
            msg->setMType(mtype);
            return true;
        }
        //十分重要的，且易错的点，先计算总长度，再进行网络序列的转化，然后再进行填充
        virtual std::string serialize(const BaseMessage::ptr &msg) override
        {
            // |--Len--|--mtype--|--idlen--|--id--|--body--|
            std::string body = msg->serialize();
            std::string id = msg->id();
            auto mtype = htonl((int32_t)msg->mtype());
            int32_t idlen = htonl(id.size());
            int32_t h_total_len = mtypeFieldsLength + idlenFieldsLength + id.size() + body.size();
            int32_t n_total_len = htonl(h_total_len);
            // DLOG("h_total_len:%d", h_total_len);
            std::string result;
            result.reserve(h_total_len);
            result.append((char *)&n_total_len, lenFieldsLength);
            result.append((char *)&mtype, mtypeFieldsLength);
            result.append((char *)&idlen, idlenFieldsLength);
            result.append(id);
            result.append(body);
            return result;
        }

    private:
        const size_t lenFieldsLength = 4;
        const size_t mtypeFieldsLength = 4;
        const size_t idlenFieldsLength = 4;
    };
    class ProtocolFactory
    {
    public:
        template <typename... Args>
        static BaseProtocol::ptr create(Args &&...args)
        {
            return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
        }
    };

    class MuduoConnection : public BaseConnection
    {
    public:
        using ptr = std::shared_ptr<MuduoConnection>;
        MuduoConnection(const muduo::net::TcpConnectionPtr &conn, const BaseProtocol::ptr &protocol)
            :_conn(conn) 
            ,_protocol(protocol)
        {
        }
        virtual void send(const BaseMessage::ptr &msg) override
        {
            std::string body = _protocol->serialize(msg);
            _conn->send(body);
        }
        virtual void shutdown() override
        {
            _conn->shutdown();
        }
        virtual bool connected() override
        {
            return _conn->connected();
        }

    private:
        BaseProtocol::ptr _protocol;
        muduo::net::TcpConnectionPtr _conn;
    };
    class ConnectionFactory
    {
    public:
        template <typename... Args>
        static BaseConnection::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
        }
    };

    class MuduoServer : public BaseServer
    {
    public:
        using ptr = std::shared_ptr<MuduoServer>;
        MuduoServer(int port)
            : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),\
                "MuduoServer", muduo::net::TcpServer::kReusePort)
            ,_protocol(ProtocolFactory::create())
        {
        }

        virtual void start() override
        {
            // 设置连接事件（连接建立/管理）的回调
            _server.setConnectionCallback(std::bind(&MuduoServer::onConnection, this, std::placeholders::_1));
            // 设置连接消息的回调
            _server.setMessageCallback(std::bind(&MuduoServer::onMessage, this,\
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            _server.start();  // 先开始监听
            _baseloop.loop(); // 开始死循环事件监视
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                cout << "连接建立!\n";
                auto muduo_conn =ConnectionFactory::create(conn,_protocol);
                //加锁
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _conns.insert(make_pair(conn,muduo_conn));
                }
                if(_cb_connection) _cb_connection(muduo_conn);
            }
            else
            {
                cout << "连接断开!\n";
                BaseConnection::ptr muduo_conn;
                //加锁
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if(it == _conns.end()){
                        return;
                    }
                    muduo_conn = it->second;
                    _conns.erase(conn);
                }
                if(_cb_close) _cb_close(muduo_conn);
            }
        }

        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            auto base_buf = BufferFactory::create(buf);
            while(1)
            {
                //防备一种情况：
                //缓冲区中数据很多很多，但是因为数据错误，导致数据又不足一条完整的消息
                //也就是一条消息过大
                //针对这种情况，直接关闭连接
                if(_protocol->canProcessed(base_buf) == false)
                {
                    //数据不足
                    if(base_buf->readableSize() > maxDataSize)
                    {
                        conn->shutdown();
                        ELOG("缓冲区中数据过大！");
                        return ;
                    }
                    break;
                }
                BaseMessage::ptr msg;
                bool ret = _protocol->onMessage(base_buf,msg);
                if(ret == false)
                {
                    conn->shutdown();
                    ELOG("缓冲区中数据错误！");
                    return ;
                }
                BaseConnection::ptr base_conn;
                // 加锁
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it == _conns.end())
                    {
                        conn->shutdown();
                        return;
                    }
                    base_conn = it->second;
                }
                if(_cb_message) _cb_message(base_conn,msg);
            }
        }

    private:
        const size_t maxDataSize = (1 << 16);
        BaseProtocol::ptr _protocol;
        muduo::net::EventLoop _baseloop; // 要注意先定义_baseloop，再定义_server
        muduo::net::TcpServer _server;
        std::mutex _mutex;
        std::unordered_map<muduo::net::TcpConnectionPtr,BaseConnection::ptr> _conns;
    };

    class MuduoServerFactory
    {
    public:
        template <typename... Args>
        static BaseServer::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
        }
    };

    class MuduoClient : public BaseClient
    {
    public:
        using ptr = std::shared_ptr<MuduoClient>;

        // MuduoClient(const std::string &server_ip, int server_port)
        //     : _protocol(ProtocolFactory::create())
        //     , _client(_baseloop, muduo::net::InetAddress(server_ip, server_port),"MuduoClient")
        //     , _downlatch(1) // 初始化计数器为1，因为为0时才会唤醒  
        //     ,_baseloop(_loopthread.startLoop())
        // {
        // }
        MuduoClient(const std::string &server_ip, int server_port)
        : _protocol(ProtocolFactory::create())
        , _downlatch(1)
        , _loopthread()
        , _baseloop(_loopthread.startLoop())  // 先初始化_baseloop，确保非空
        , _client(_baseloop, muduo::net::InetAddress(server_ip, server_port),"MuduoClient")
        {
            // 关键：校验_baseloop是否为空，避免TcpClient构造失败
            if (_baseloop == nullptr) {
                ELOG("EventLoopThread启动失败，_baseloop为空！");
                abort(); // 直接终止，避免后续段错误
            }
        }
        virtual void connect() override
        {
            // 设置连接事件（连接建立/管理）的回调
            _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this, std::placeholders::_1));
            // 设置连接消息的回调
            _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this, \
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 连接服务器
            _client.connect();
            _downlatch.wait();
        }
        virtual void shutdown() override
        {
            return _client.disconnect();
        }
        virtual bool send(const BaseMessage::ptr &msg) override
        {
            if(connected() == false)
            {
                ELOG("连接已断开！");
                return false;
            }
            _conn->send(msg);
            return true;
        }
        virtual BaseConnection::ptr connection()  override
        {
            return _conn;
        }
        virtual bool connected()  override
        {
            return (_conn && _conn->connected());
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                cout << "连接建立!\n";
                _downlatch.countDown(); // 计数减减，为0时唤醒阻塞
                _conn = ConnectionFactory::create(conn,_protocol);
            }
            else
            {
                cout << "连接断开!\n";
                _conn.reset();
            }
        }

        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            auto base_buf = BufferFactory::create(buf);
            while(1)
            {
                //防备一种情况：
                //缓冲区中数据很多很多，但是因为数据错误，导致数据又不足一条完整的消息
                //也就是一条消息过大
                //针对这种情况，直接关闭连接
                if(_protocol->canProcessed(base_buf) == false)
                {
                    //数据不足
                    if(base_buf->readableSize() > maxDataSize)
                    {
                        conn->shutdown();
                        ELOG("缓冲区中数据过大！");
                        return ;
                    }
                    break;
                }
                BaseMessage::ptr msg;
                bool ret = _protocol->onMessage(base_buf,msg);
                if(ret == false)
                {
                    conn->shutdown();
                    ELOG("缓冲区中数据错误！");
                    return ;
                }
                if(_cb_message) _cb_message(_conn,msg);
            }
        }

    private:
        const size_t maxDataSize = (1 << 16);
        BaseProtocol::ptr _protocol;
        BaseConnection::ptr _conn;
        muduo::CountDownLatch _downlatch;
        muduo::net::EventLoopThread _loopthread;
        muduo::net::EventLoop *_baseloop;
        muduo::net::TcpClient _client;
    };

    class MuduoClientFactory
    {
    public:
        template <typename... Args>
        static BaseClient::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
        }
    };
}