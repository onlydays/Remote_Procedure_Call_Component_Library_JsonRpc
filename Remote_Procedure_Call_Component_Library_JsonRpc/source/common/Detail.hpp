/*
    实现项目中用到的一些琐碎功能代码
    *   日志宏的定义
    *   json的序列化和反序列化
    *   uuid的生成
*/

#pragma once

#include <iostream>
#include <cstdio>
#include <ctime>

#include <sstream>
#include <string>
#include <memory.h>
#include <jsoncpp/json/json.h>

#include <chrono>
#include <random>
#include <atomic>
#include <iomanip>

namespace only_days
{
// 1.日志宏的定义
#define LDBG 0
#define LINF 1
#define LERR 2

#define LDEFAULT LDBG

//%T == (%H:%M:%S)
#define LOG(level, format, ...)                                                                       \
    {                                                                                                 \
        if (level >= LDEFAULT)                                                                        \
        {                                                                                             \
            time_t t = time(NULL);                                                                    \
            struct tm *lt = localtime(&t);                                                            \
            char time_tmp[32] = {0};                                                                  \
            strftime(time_tmp, 31, "%m-%d %T", lt);                                                   \
            fprintf(stdout, "[%s][%s:%d] " format "\n", time_tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
        }                                                                                             \
    }
#define DLOG(format, ...) LOG(LDBG, format, ##__VA_ARGS__);
#define LLOG(format, ...) LOG(LINF, format, ##__VA_ARGS__);
#define ELOG(format, ...) LOG(LERR, format, ##__VA_ARGS__);

    // 2.json的序列化和反序列化
    class JSON
    {
    public:
        // 实现数据的序列化
        //使用静态成员函数，使得外界能通过类名直接调用
        //如：JSON::serialize();
        static bool serialize(const Json::Value &val, std::string &content)
        {
            std::stringstream ss;
            // 先实例化一个工厂类对象
            Json::StreamWriterBuilder swb;
            // 通过工厂类对象来生产派生对象
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
            int ret = sw->write(val, &ss);
            if (ret != 0)
            {
                ELOG("json serialize failed!\n");
                return false;
            }
            content = ss.str();
            return true;
        }

        // 实现json字符串的反序列化
        static bool unserialize(const std::string &body, Json::Value &val)
        {
            // 实例化⼯⼚类对象
            Json::CharReaderBuilder crb;
            // ⽣产CharReader对象
            std::string errs;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            //parse的第一个参数是字符串的起始地址
            //第二个参数是字符串的结束地址
            //第三个参数是存放Json对象的地址
            //第四个参数是error信息
            bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), &val, &errs);
            if (ret == false)
            {
                ELOG("json unserialize failed : %s", errs.c_str());
                return false;
            }
            return true;
        }
    };

    // 3.uuid的生成
    class UUID
    {
    public:
        static std::string uuid()
        {
            std::stringstream ss;
            // 1. 构造⼀个机器随机数对象
            std::random_device rd;
            // 2. 以机器随机数为种⼦构造伪随机数对象
            std::mt19937 generator(rd());
            // 3. 构造限定数据范围的对象
            std::uniform_int_distribution<int> distribution(0, 255);
            // UUID的标准型式包含32个16进制数字字符，以连字号分为五段
            // 形式为8-4-4-4-12的32个字符
            // 如：550e8400-e29b-41d4-a716-446655440000。(16个字节)
            //  4. ⽣成8个随机数，按照特定格式组织成为16进制数字字符的字符串
            for (int i = 0; i < 8; i++)
            {
                if (i == 4 || i == 6)
                    ss << "-";
                // 00-ff
                ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
            }
            ss << "-";
            // 5. 定义⼀个8字节序号，逐字节组织成为16进制数字字符的字符串
            static std::atomic<size_t> seq(1); // 00 00 00 00 00 00 00 01
            size_t cur = seq.fetch_add(1);
            for (int i = 7; i >= 0; i--)
            {
                if (i == 5)
                    ss << "-";
                //((cur >> (i * 8)) & 0xFF)进行移位操作
                ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (i * 8)) & 0xFF);
            }
            return ss.str();
        }
    };
}
