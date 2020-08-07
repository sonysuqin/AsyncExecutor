#ifndef __IO_SERVICE_H__
#define __IO_SERVICE_H__

#include <future>
#include <memory>
#include <thread>

#include "function_view.h"
#include "timer.h"

// Forward declaration for hiding boost headers.
namespace boost {
namespace asio {
typedef class io_context io_service;
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

  int wait() {
    std::future<int> future = promise_.get_future();
    return future.get();
  }

 private:
  FunctionView<void()> functor_;
  std::promise<int> promise_;
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

// This class makes use of boost asio io_service, but hide all boost context.
class IOService {
 public:
  IOService();
  ~IOService();

 public:
  // Start io_service thread and run loop.
  bool Start();

  // Stop io_service thread and run loop.
  bool Stop();

  // Return if current thread is the io_service thread.
  bool IsCurrent();

  // Return if io_service thread running.
  bool Running() { return running_; }

  // Sync call a method |functor| with a return value, the |functor|
  // will be executed on io_service thread.
  template <
      class ReturnT,
      typename = typename std::enable_if<!std::is_void<ReturnT>::value>::type>
  ReturnT Invoke(FunctionView<ReturnT()> functor) {
    ReturnT result;
    InvokeInternal([functor, &result] { result = functor(); });
    return result;
  }

  // Sync call a method |functor| without return value, the |functor|
  // will be executed on io_service thread.
  template <
      class ReturnT,
      typename = typename std::enable_if<std::is_void<ReturnT>::value>::type>
  void Invoke(FunctionView<void()> functor) {
    InvokeInternal(functor);
  }

  // Post a task |functor| to io_service thread and return immediately.
  template <class FunctorT>
  void PostTask(FunctorT&& functor) {
    FunctorWrapper* p =
        new FunctorPost<FunctorT>(std::forward<FunctorT>(functor));
    std::shared_ptr<FunctorWrapper> functor_wrapper(p);
    PostInternal(functor_wrapper);
  }

  // Create a timer bound to this io_service, but IOService doesn't
  // own the timer, make sure the timer be closed before IOService
  // stopped.
  std::shared_ptr<Timer> CreateTimer();

 protected:
  void SetCurrent();
  void InvokeInternal(FunctionView<void()> functor);
  void PostInternal(std::shared_ptr<FunctorWrapper> functor_wrapper);

 protected:
  std::shared_ptr<boost::asio::io_service> ios_;
  std::shared_ptr<boost::asio::io_service_work> work_;
  std::shared_ptr<std::thread> thread_;
  volatile std::atomic_bool running_;
  static thread_local IOService* self_;
};

}  // namespace bee

#endif  // __IO_SERVICE_H__
