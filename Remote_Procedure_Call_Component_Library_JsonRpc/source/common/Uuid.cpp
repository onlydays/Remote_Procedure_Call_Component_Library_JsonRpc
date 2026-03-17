#pragma once
#include<iostream>
#include <chrono>
#include <random>
#include <string>
#include <sstream>
#include <atomic>
#include <iomanip>

std::string uuid()
{
    std::stringstream ss;
    // 1. 构造⼀个机器随机数对象
    std::random_device rd;
    // 2. 以机器随机数为种⼦构造伪随机数对象
    std::mt19937 generator(rd());
    // 3. 构造限定数据范围的对象
    std::uniform_int_distribution<int> distribution(0, 255);
    //UUID的标准型式包含32个16进制数字字符，以连字号分为五段
    //形式为8-4-4-4-12的32个字符
    //如：550e8400-e29b-41d4-a716-446655440000。(16个字节)
    // 4. ⽣成8个随机数，按照特定格式组织成为16进制数字字符的字符串
    for (int i = 0; i < 8; i++)
    {
        if (i == 4 || i == 6)
            ss << "-";
        //00-ff
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

int main()
{
    for (int i = 0; i < 10; i++)
    {
        std::cout << uuid() << std::endl;
    }
    return 0;
}