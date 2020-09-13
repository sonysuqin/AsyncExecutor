#include <stdio.h>
#include <chrono>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "io_service.h"

#include "xlog/log/xlogger.h"
#include "xlog/log/appender.h"

#define LEAK_CHECK
#ifdef LEAK_CHECK
#ifdef WIN32
#include <vld.h>
#endif
#endif

#ifdef UNIT_TEST

#ifdef _DEBUG
#pragma comment(lib, "gtestd.lib")
#else
#pragma comment(lib, "gtest.lib")
#endif

#include "gtest/gtest.h"

#endif  // UNIT_TEST

using namespace bee;

std::shared_ptr<HttpEngine> http_engine(new HttpEngine);
std::shared_ptr<IOService> executor(new IOService(http_engine));

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
          "{\"cmd\":\"create\",\"roomid\":"
          "\"3866b18782bdd9f82a201801a06df2bba7a2ec865a54b5d30ab18adf\","
          "\"uid\":\"11\"}";
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

std::shared_ptr<Http> http;

class HttpSink : public HttpCallback {
 public:
  // virtual functions.
  virtual void OnRedirectReceived(Cronet_UrlRequestPtr request,
                                  Cronet_UrlResponseInfoPtr info,
                                  Cronet_String newLocationUrl) {
    printf("OnRedirectReceived %s\n", newLocationUrl);
    http->FollowRedirect();
  }

  virtual void OnResponseStarted(Cronet_UrlRequestPtr request,
                                 Cronet_UrlResponseInfoPtr info) {
    std::cout << "OnResponseStarted called." << std::endl;
    std::cout << "HTTP Status: "
              << Cronet_UrlResponseInfo_http_status_code_get(info) << " "
              << Cronet_UrlResponseInfo_http_status_text_get(info) << std::endl;
    // Create and allocate 32kb buffer.
    Cronet_BufferPtr buffer = Cronet_Buffer_Create();
    Cronet_Buffer_InitWithAlloc(buffer, 32 * 1024);
    // Started reading the response.
    http->Read(buffer);
  }

  virtual void OnReadCompleted(Cronet_UrlRequestPtr request,
                               Cronet_UrlResponseInfoPtr info,
                               Cronet_BufferPtr buffer,
                               uint64_t bytes_read) {
    printf("Receive %llu bytes\n", bytes_read);
    std::string last_read_data(
        reinterpret_cast<char*>(Cronet_Buffer_GetData(buffer)),
        (size_t)bytes_read);
    data_ += last_read_data;
    http->Read(buffer);
  }

  virtual void OnSucceeded(Cronet_UrlRequestPtr request,
                           Cronet_UrlResponseInfoPtr info) {
    std::cout << "OnSucceeded called." << std::endl;
    printf("Http response:%s\n", data_.c_str());
  }

  virtual void OnFailed(Cronet_UrlRequestPtr request,
                        Cronet_UrlResponseInfoPtr info,
                        Cronet_ErrorPtr error) {
    std::cout << "OnFailed called: " << Cronet_Error_message_get(error)
              << std::endl;
  }

  virtual void OnCanceled(Cronet_UrlRequestPtr request,
                          Cronet_UrlResponseInfoPtr info) {
    std::cout << "OnCanceled called." << std::endl;
  }

  std::string data_;
};

unsigned long long getTid() {
  std::ostringstream oss;
  oss << std::this_thread::get_id();
  std::string stid = oss.str();
  unsigned long long tid = std::stoull(stid);
  return tid;
}

class TestTls {
 public:
  TestTls() { printf("[%lld] TestTls created\n", getTid()); }
  ~TestTls() { printf("[%lld] TestTls deleted\n", getTid()); }
};

class TlsContext {
 public:
    static thread_local TestTls test_tls;
};

thread_local TestTls TlsContext::test_tls;

int main(int argc, char* argv[]) {
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
    timer->Open(100, true, [] { printf("[Exector:%ld] haha\n", GetTid()); });

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

  if (0) {
    executor->Start();
    ws = executor->CreateWebSocket();
    auto sink = std::make_shared<WsSink>();
    ws->Open("ws://name.hd.sohu.com/bee_msg_media", std::vector<std::string>(),
             sink);

    // getchar();
    printf("Press enter to close.\n");
    getchar();
    ws->Close();
    ws = nullptr;
    sink = nullptr;
    printf("Ws closed, press enter to continue.\n");
    getchar();

    http = executor->CreateHttp();
    auto http_sink = std::make_shared<HttpSink>();
    http->Get("http://roblin.cn/video/qyn/33.mp4", http_sink);
    getchar();
    http->Close();
    printf("Http closed, press enter to continue.\n");
    getchar();

    // executor->Stop();
    // executor = nullptr;
  }

#ifdef UNIT_TEST
  testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
#endif

  if (0) {
    std::thread t1([] { printf("[%lld] thread1 in\n", getTid()); });

    std::thread t2([] { printf("[%lld] thread2 in\n", getTid()); });
  }

  if (1) {
      xlogger_SetLevel(kLevelDebug);
      appender_set_console_log(true);
      appender_open(kAppednerSync, "H:\\log\\roblin", "xlog_test", "");
      for (int i = 0; i < 100; ++i) {
        xinfo2("Hello %d", i);
      }
      appender_flush_sync();
  }

  printf("About to exit.\n");

  getchar();
  return 0;
}
