
#include<stdio.h>
#include<time.h>

//2.#define LOG(msg) printf("%s\n",msg)
//3.#define LOG(format,msg) printf(format"\n",msg)
//其中__FILE__和__LINE__是预编译选项，在编译器编译的时候会分别替换为文件名和对应的行号
//4.#define LOG(format,msg) printf("[%s:%d] "format"\n",__FILE__,__LINE__,msg)

//5.
//%T == (%H:%M:%S)
// #define LOG(format,msg){\
//     time_t t =time(NULL);\
//     struct tm* lt =localtime(&t);\
//     char time_tmp[32] = {0};\
//     strftime(time_tmp,31,"%m-%d %T",lt);\
//     printf("[%s][%s:%d] "format"\n",time_tmp,__FILE__,__LINE__,msg);\
// }
//localtime和strftime函数


//6.添加"..."不定参，然后参数部分改为__VA_ARGS__
//%T == (%H:%M:%S)
// #define LOG(format,...){\
//     time_t t =time(NULL);\
//     struct tm* lt =localtime(&t);\
//     char time_tmp[32] = {0};\
//     strftime(time_tmp,31,"%m-%d %T",lt);\
//     printf("[%s][%s:%d] "format"\n",time_tmp,__FILE__,__LINE__,__VA_ARGS__);\
// }

//7.在__VA_ARGS__前面加两个'#'，进行特殊处理
//%T == (%H:%M:%S)
// #define LOG(format,...){\
//     time_t t =time(NULL);\
//     struct tm* lt =localtime(&t);\
//     char time_tmp[32] = {0};\
//     strftime(time_tmp,31,"%m-%d %T",lt);\
//     printf("[%s][%s:%d] "format"\n",time_tmp,__FILE__,__LINE__,##__VA_ARGS__);\
// }

#define LDBG 0
#define LINF 1
#define LERR 2

#define LDEFAULT LDBG

//8.添加日志等级
//%T == (%H:%M:%S)
#define LOG(level, format, ...){\
    if(level >= LDEFAULT){\
    time_t t =time(NULL);\
    struct tm* lt =localtime(&t);\
    char time_tmp[32] = {0};\
    strftime(time_tmp,31,"%m-%d %T",lt);\
    printf("[%s][%s:%d] "format"\n",time_tmp,__FILE__,__LINE__,##__VA_ARGS__);\
    }\
}
#define DLOG(format, ...) LOG(LDBG, format, ##__VA_ARGS__);
#define LLOG(format, ...) LOG(LINF, format, ##__VA_ARGS__);
#define ELOG(format, ...) LOG(LERR, format, ##__VA_ARGS__);


//9.打印到文件里
//%T == (%H:%M:%S)
//fprintf中的第一个参数可以换成其他文件的文件描述符
#define LOG(level, format, ...){\
    if(level >= LDEFAULT){\
    time_t t =time(NULL);\
    struct tm* lt =localtime(&t);\
    char time_tmp[32] = {0};\
    strftime(time_tmp,31,"%m-%d %T",lt);\
    fprintf(stdout,"[%s][%s:%d] "format"\n",time_tmp,__FILE__,__LINE__,##__VA_ARGS__);\
    }\
}

int main()
{
    //1.printf("%s\n","hello world!");
    //2.LOG("hello world!");
    //3.LOG("%d",99);
    //4.LOG("%d",99);
    //5.LOG("%d",99);
    //6.LOG("%s:%d","hello world!",99);
    //7.LOG("hello world!");
    //8.
    DLOG("hello world!");
    LLOG("hello world!");
    ELOG("hello world!");
    
    return 0;
}