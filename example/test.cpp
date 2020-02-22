#include <stdio.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "executor.h"

#ifdef LEAK_CHECK
#ifdef WIN32
#include <vld.h>
#endif
#endif

using namespace qrtc;

Executor executor;

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
    Test t;
    // Recursive test.
    executor.Invoke<void>(std::bind(&Test::print1, &t));
    // Recursive test.
    int r = executor.Invoke<int>(std::bind(&Test::fun3, &t));
    printf("[Exector:%ld] Invoke fun3() return %d\n", GetTid(), r);
    printf("[Exector:%ld] Test::fun1() out\n\n", GetTid());
  }

  int fun2() {
    printf("[Exector:%ld] Test::fun2() in\n", GetTid());
    Test t;
    // Recursive test.
    executor.PostTask(std::bind(&Test::print2, &t));
    printf("[Exector:%ld] Test::fun2() out\n\n", GetTid());
    return 2;
  }

  int fun3() { return 3; }
};

int main() {
  printf("[Main:%ld] Main thread.\n", GetTid());
  executor.Start();
  if (1) {
    Test t;
    printf("[Main:%ld] PostTask fun1().\n", GetTid());
    executor.PostTask(std::bind(&Test::fun1, &t));
    printf("[Main:%ld] Invoke fun2().\n", GetTid());
    int i = executor.Invoke<int>(std::bind(&Test::fun2, &t));
    printf("[Main:%ld] Invoke fun2() return %d\n", GetTid(), i);
  }
  executor.Stop();
  getchar();
  return 0;
}
