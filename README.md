# 1 说明
这是一个简单的模块，用于在C++的代码中，从一个线程同步或者异步的在另外一个线程中调用一个方法。

# 2 调用方法
```
// 示例类
class Test {
 public:
  int fun1() {
    return 1;
  }
  void fun2() {
    printf("fun2() called.\n");
  }
};

// 创建一个简单对象.
Test t;

// 创建一个Executor对象.
Executor executor;

// 启动线程.
executor.Start();

// 以下的接口方式直接copy自WebRTC.

// 同步调用，fun1()在executor的线程中执行，当前线程阻塞等待返回值.
int ret = executor.Invoke<int>(std::bind(&Test::fun1, &t));
printf("Invoke fun1 return %d.\n", ret);

// 异步调用，fun2()在executor的线程中执行，当前线程不阻塞直接返回.
executor.PostTask(std::bind(&Test::fun2, &t));

// 关闭线程.
executor.Stop();
```

# 3 WHY?
总会有那样的需求, 要把调用扔到同一个线程里. C++11提供了std::async, 可悲的是居然不能指定线程, 也不确保创建线程池, 名副其实的"鸡肋".最后只好自己造轮子，很多第三方库都有自己的实现, Boost的Asio, Chromium, WebRTC等开源库都有自己的实现. 这些库的实现都是在一个线程中绑定一个消息队列, 外部线程扔一个消息进去, 内部线程通过消息循环处理.

为了使用这个特性没有必要整个地引用这些库, 这个模块拼凑了几个文件, 用线程+队列封装了一个简单的实现, 基本完全基于标准C++(除了信号量, 信号量也已经支持Window、Linux(Android)、iOS). 计划将之作为另一项目的核心模块长期维护, 如果有兴趣测试的同学发现问题, 欢迎报告问题, 非常感谢!!

# 4 怎么使用?
直接引入代码.
