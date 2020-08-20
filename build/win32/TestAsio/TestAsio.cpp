#include <stdio.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <stdio.h>

#include "io_service.h"

#define LEAK_CHECK
#ifdef LEAK_CHECK
#ifdef WIN32
#include <vld.h>
#endif
#endif

using namespace bee;

IOService* executor = new IOService;

long GetTid() {
  std::ostringstream oss;
  oss << std::this_thread::get_id();
  std::string stid = oss.str();
  long tid = std::stol(stid);
  return tid;
}

class Test {
 public:
  Test() {}
  ~Test() {}

  void print1() { printf("[Exector:%ld] print1\n", GetTid()); }

  void print2() { printf("[Exector:%ld] print2\n", GetTid()); }

  void fun1() {
    printf("[Exector:%ld] Test::fun1() in\n", GetTid());
    // Recursive test.
    executor->Invoke<void>(std::bind(&Test::print1, this));
    // Recursive test.
    int r = executor->Invoke<int>(std::bind(&Test::fun3, this));
    printf("[Exector:%ld] Invoke fun3() return %d\n", GetTid(), r);
    printf("[Exector:%ld] Test::fun1() out\n\n", GetTid());
  }

  int fun2() {
    printf("[Exector:%ld] Test::fun2() in\n", GetTid());
    // Recursive test.
    executor->PostTask(std::bind(&Test::print2, this));
    printf("[Exector:%ld] Test::fun2() out\n\n", GetTid());
    return 2;
  }

  int fun3() { return 3; }
};

void test2() {
  printf("test2\n");
}

void test1() {
  printf("test1\n");
  executor->PostTask(std::bind(&test2));
}

int main() {
  printf("[Main:%ld] Main thread.\n", GetTid());
  while (1) {
    executor->Start();
    printf("started\n");
    getchar();
    int count = 100;
    int index = 0;
    Test t;
    while (index++ < count) {
      printf("[Main:%ld] PostTask fun1().\n", GetTid());
      executor->PostTask(std::bind(&Test::fun1, &t));
      printf("[Main:%ld] Invoke fun2().\n", GetTid());
      int i = executor->Invoke<int>([&t] { return t.fun2(); });
      printf("[Main:%ld] Invoke fun2() return %d\n", GetTid(), i);
      printf("Test %d\n", index);
    }

    uint64_t timer = executor->OpenTimer(
        100, true, [] { printf("[Exector:%ld] haha\n", GetTid()); });

    executor->Invoke<void>(std::bind(&test1));
    getchar();
    executor->Stop();
    printf("stopped\n");
    getchar();
  }
  return 0;
}
