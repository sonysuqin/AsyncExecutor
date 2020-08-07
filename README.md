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

// 创建一个IOService对象.
IOService* executor = new IOService;

// 启动线程.
executor->Start();

// 同步调用，fun1()在executor的线程中执行，当前线程阻塞等待返回值.
int ret = executor->Invoke<int>(std::bind(&Test::fun1, &t));
printf("Invoke fun1 return %d.\n", ret);

// 异步调用，fun2()在executor的线程中执行，当前线程不阻塞直接返回.
executor->PostTask(std::bind(&Test::fun2, &t));

// 定时器.
std::shared_ptr<bee::Timer> timer = executor->CreateTimer();
timer->Open(1000, true, [] { printf("[Exector:%ld] haha\n", GetTid()); });
  
// 关闭线程.
executor->Stop();

// 删除IOService对象.
delete executor;
```

# 3 怎么使用?
直接引入代码.
