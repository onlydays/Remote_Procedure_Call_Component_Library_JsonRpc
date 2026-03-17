#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/TcpClient.h>
#include<muduo/net/TcpConnection.h>
#include<muduo/net/Buffer.h>
#include<muduo/base/CountDownLatch.h>
#include<muduo/net/EventLoopThread.h>
#include<iostream>
#include<string>

using std::cout;
using std::endl;

class DictClient{
public:
    DictClient(const std::string &server_ip,int server_port)
    :_client(_baseloop,muduo::net::InetAddress(server_ip,server_port),"DictClient")
    ,_downlatch(1)//初始化计数器为1，因为为0时才会唤醒
    ,_baseloop(_loopthread.startLoop())
    {
         //设置连接事件（连接建立/管理）的回调
        _client.setConnectionCallback(std::bind(& DictClient::onConnection,this,std::placeholders::_1));
        //设置连接消息的回调
        _client.setMessageCallback(std::bind(& DictClient::onMessage,this,
        std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

        //连接服务器
        _client.connect();
        _downlatch.wait();
    }

    bool send(const std::string &msg)
    {
        if(_conn->connected() == false){
            cout<<"连接已断开，发送数据失败!\n";
            return false;
        }
        _conn->send(msg);
        //开始事件循环监控---内部是个死循环
        //对于客户端来说，不能直接使用，因为一但开始循环，就会一直卡在这里
        //引入EventLoopThread,所以不需要baseloop了
        return true;
    }

private:
     void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if(conn->connected()){
            cout<<"连接建立!\n";
            _downlatch.countDown();//计数减减，为0时唤醒阻塞
            _conn = conn;
        }else{
            cout<<"连接断开!\n";
            _conn.reset();
        }
    }

    void onMessage(const muduo::net::TcpConnectionPtr &conn,muduo::net::Buffer *buf,muduo::Timestamp)
    {
        std::string res = buf->retrieveAllAsString();
        cout<<res<<endl;
    }
private:
    muduo::net::TcpConnectionPtr _conn;
    muduo::CountDownLatch _downlatch;
    muduo::net::EventLoopThread _loopthread;
    muduo::net::EventLoop *_baseloop;
    muduo::net::TcpClient _client;
};

int main()
{
    DictClient client("127.0.0.1",9090);
    while(1)
    {
        std::string msg;
        std::cin >> msg;
        client.send(msg);
    }
    return 0;
}