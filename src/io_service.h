#ifndef BEE_IO_SERVICE_H
#define BEE_IO_SERVICE_H

#include <future>
#include <memory>
#include <thread>
#include <unordered_map>

#include "function_view.h"
#include "http_factory.h"
#include "timer_factory.h"
#include "websocket.h"
#include "websocket_factory.h"

// Forward declaration for hiding boost headers.
namespace boost {
namespace asio {
class io_context;
class io_service_work;
}  // namespace asio
}  // namespace boost

namespace bee {

// Functor wrapper base.
class FunctorWrapper {
 public:
  virtual ~FunctorWrapper() {}
  virtual void run() = 0;

 protected:
  FunctorWrapper() {}

 private:
  FunctorWrapper(const FunctorWrapper&) = delete;
  FunctorWrapper& operator=(const FunctorWrapper&) = delete;
};

// Functor wrapper for sync invoke.
class FunctorInvoker : public FunctorWrapper {
 public:
  explicit FunctorInvoker(FunctionView<void()> functor) : functor_(functor) {}
  ~FunctorInvoker() {}

  void run() override {
    functor_();
    promise_.set_value(0);
  }

  int32_t wait() {
    std::future<int32_t> future = promise_.get_future();
    return future.get();
  }

 private:
  FunctionView<void()> functor_;
  std::promise<int32_t> promise_;
};

// Functor wrapper for async post.
template <class FunctorT>
class FunctorPost : public FunctorWrapper {
 public:
  explicit FunctorPost(FunctorT&& functor)
      : functor_(std::forward<FunctorT>(functor)) {}

  void run() override { functor_(); }

 private:
  ~FunctorPost() override {}
  typename std::remove_reference<FunctorT>::type functor_;
};

// This class makes use of boost asio io_context, but hide all boost context.
// Note that all io objects created from IOService such as timer and websocket
// must be closed and deleted before IOService is deleted, for they depend on
// io_context of IOService.
class IOService : public std::enable_shared_from_this<IOService>,
                  public HttpExecutor,
                  public HttpFactory,
                  public TimerFactory,
                  public WebSocketFactory {
 public:
  IOService(std::shared_ptr<HttpEngine> http_engine = nullptr);
  ~IOService();

 public:
  // Start io_context thread and run loop.
  bool Start();

  // Stop io_context thread and run loop, never stop thread in thread itself.
  bool Stop();

  // Return if current thread is the io_context thread.
  bool IsCurrent();

  // Return if io_context thread running.
  bool Running() { return running_; }

  // Sync call a method |functor| with a return value, the |functor|
  // will be executed on io_context thread.
  template <
      class ReturnT,
      typename = typename std::enable_if<!std::is_void<ReturnT>::value>::type>
  ReturnT Invoke(FunctionView<ReturnT()> functor) {
    ReturnT result;
    InvokeInternal([functor, &result] { result = functor(); });
    return result;
  }

  // Sync call a method |functor| without return value, the |functor|
  // will be executed on io_context thread.
  template <
      class ReturnT,
      typename = typename std::enable_if<std::is_void<ReturnT>::value>::type>
  void Invoke(FunctionView<void()> functor) {
    InvokeInternal(functor);
  }

  // Post a task |functor| to io_context thread and return immediately.
  template <class FunctorT>
  void PostTask(FunctorT&& functor) {
    FunctorWrapper* p =
        new FunctorPost<FunctorT>(std::forward<FunctorT>(functor));
    std::shared_ptr<FunctorWrapper> functor_wrapper(p);
    PostInternal(functor_wrapper);
  }

  // HttpExecutor implementation.
  void ExecuteRunnable(Cronet_RunnablePtr runnable) override;

  void ExecuteFunction(std::function<void()> function) override;

  bool IsExecutorThread() override;

  // HttpFactory implementation.
  std::shared_ptr<Http> CreateHttp() override;

  // WebSocketFactory implementation.
  std::shared_ptr<WebSocket> CreateWebSocket() override;

  // TimerFactory implementation.
  std::shared_ptr<Timer> CreateTimer() override;

 protected:
  void InitCurrentThread();
  void UnInitCurrentThread();
  void InvokeInternal(FunctionView<void()> functor);
  void PostInternal(std::shared_ptr<FunctorWrapper> functor_wrapper);

 protected:
  std::shared_ptr<boost::asio::io_context> ioc_;
  std::unique_ptr<boost::asio::io_service_work> work_;
  std::unique_ptr<std::thread> thread_;
  // Expect all IOService objects hold a single global HttpEngine,
  // so they can shared all cache, connection contexts, etc.
  std::shared_ptr<HttpEngine> http_engine_;
  volatile std::atomic_bool running_;
  static thread_local IOService* self_;
};

}  // namespace bee

#endif  // BEE_IO_SERVICE_H
