#pragma once
#include<iostream>
#include<string>
#include<unordered_map>

//rcode == return code
namespace only_days
{
    //下列宏定义为请求字段宏定义
    #define KEY_METHOD "method"
    #define KEY_PARAMS "parameters"
    #define KEY_TOPIC_KEY "topic_key"
    #define KEY_TOPIC_MSG "topic_msg"
    #define KEY_OPTYPE "optype"
    #define KEY_HOST "host"
    #define KEY_HOST_IP "ip"
    #define KEY_HOST_PORT "port"
    #define KEY_RCODE "rcode"
    #define KEY_RESULT "result"

    //下列MType的枚举为消息类型
    //rpc请求&响应  1~2
    //主题操作请求&响应 3~4
    //服务操作请求&响应 5~6
    enum class MType {
        REQ_RPC = 0,
        RSP_RPC,
        REQ_TOPIC,
        RSP_TOPIC,
        REQ_SERVICE,
        RSP_SERVICE
    };

    //响应码类型定义
    enum class RCode
    {
        RCODE_OK = 0,
        RCODE_PARSE_FAILED,
        RCODE_ERROR_MSGTYPE,
        RCODE_INVALID_MSG,
        RCODE_DISCONNECTED,
        RCODE_INVALID_PARAMS,
        RCODE_NOT_FOUND_SERVICE,
        RCODE_INVALID_OPTYPE,
        RCODE_NOT_FOUND_TOPIC,
        RCODE_INTERNAL_ERROR
    };

    //显示每个响应码对应的的原因
    static std::string errReason(RCode code)
    {
        static std::unordered_map<RCode, std::string> err_map = {
            {RCode::RCODE_OK, "成功处理！"},
            {RCode::RCODE_PARSE_FAILED, "消息解析失败！"},
            {RCode::RCODE_ERROR_MSGTYPE, "消息类型错误！"},
            {RCode::RCODE_INVALID_MSG, "⽆效消息"},
            {RCode::RCODE_DISCONNECTED, "连接已断开！"},
            {RCode::RCODE_INVALID_PARAMS, "⽆效的Rpc参数！"},
            {RCode::RCODE_NOT_FOUND_SERVICE, "没有找到对应的服务！"},
            {RCode::RCODE_INVALID_OPTYPE, "⽆效的操作类型"},
            {RCode::RCODE_NOT_FOUND_TOPIC, "没有找到对应的主题！"},
            {RCode::RCODE_INTERNAL_ERROR, "内部错误！"}};

        auto it = err_map.find(code);
        if (it == err_map.end())
        {
            return "未知错误！";
        }

        return it->second;
    }

    //RPC请求类型定义
    //同步请求：等待收到响应后返回
    //异步请求：返回异步对象，在需要的时候通过异步对象获取响应结果（还未收到结果会阻塞）
    //回调请求：设置回调函数，通过回调函数对响应进⾏处理
    enum class RType
    {
        REQ_SYNC = 0,
        REQ_ASYNC,
        REQ_CALLBACK
    };

    //主题操作类型定义
    //1.主题创建
    //2.主题删除
    //3.主题订阅
    //4.主题取消订阅
    //5.主题消息发布
    enum class TopicOptype
    {
        TOPIC_CREATE = 0,
        TOPIC_REMOVE,
        TOPIC_SUBSCRIBE,
        TOPIC_CANCEL,
        TOPIC_PUBLISH
    };

    //服务操作类型定义
    //1.服务注册
    //2.服务发现
    //3.服务上线
    //4.服务下线
    enum class ServiceOptype
    {
        SERVICE_REGISTRY = 0,
        SERVICE_DISCOVERY,
        SERVICE_ONLINE,
        SERVICE_OFFLINE,
        SERVICE_UNKNOW
    };
}