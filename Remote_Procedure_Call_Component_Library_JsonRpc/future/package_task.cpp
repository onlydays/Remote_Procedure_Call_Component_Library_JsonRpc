#include<iostream>
#include<future>
#include<thread>
#include<chrono>


using std::cout;
using std::endl;

int Add(int num1,int num2)
{
    cout<<"into add!\n";
    return num1 + num2;
}

int main()
{
    //1.封装任务
    std::packaged_task<int(int,int)> task(Add);
    
    //2.获取任务包关联的future对象
    std::future<int> res = task.get_future();

    //2.执行任务
    // std::thread thr([task](){
    //     task(11,22);
    // });

    //3.获取结果
    cout<<res.get()<<endl;
    //thr.join();
    return 0;
}