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

std::shared_ptr<IOService> executor(new IOService());

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

std::shared_ptr<WebSocket> ws;

class WsSink : public WebSocketSink {
 public:
  virtual void OnOpen() {
    printf("OnOpen\n");
    executor->PostTask([] {
      std::string msg =
          "{\"cmd\":\"create\",\"roomid\":\"3866b18782bdd9f82a201801a06df2bba7a2ec865a54b5d30ab18adf\",\"uid\":\"11\"}";
      ws->Send(msg.c_str(), msg.size());
    });
  }

  virtual void OnData(const char* buffer, size_t size) {
    printf("OnData %u\n", size);
  }

  virtual void OnClose() { printf("OnClose\n"); }

  virtual void OnError(int32_t error_code, const std::string& error_message) {
    printf("OnError %d %s\n", error_code, error_message.c_str());
  }
};

int main() {
  printf("[Main:%ld] Main thread.\n", GetTid());
  while (0) {
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

    auto timer = executor->CreateTimer();
    timer->Open(
        100, true, [] { printf("[Exector:%ld] haha\n", GetTid()); });

    executor->Invoke<void>(std::bind(&test1));
    getchar();
    timer->Close();
    timer = nullptr;
    executor->Stop();
    printf("stopped\n");
    char c = getchar();
    if (c == 'q') {
      break;
    }
  }

  executor->Start();
  ws = executor->CreateWebSocket();
  auto sink = std::make_shared<WsSink>();
  ws->Open("ws://name.hd.sohu.com/bee_msg_media", std::vector<std::string>(),
           sink);

  //getchar();
  printf("Press enter to close.\n");
  getchar();
  ws->Close();
  ws = nullptr;
  printf("Closed, press enter to continue.\n");
  //getchar();
  sink = nullptr;
  executor->Stop();
  executor = nullptr;
  printf("About to exit.\n");
  getchar();
  return 0;
}
