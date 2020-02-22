#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include <atomic>
#include <future>
#include <memory>
#include <thread>

#include "function_view.h"
#include "safe_queue.h"

namespace qrtc {

class MessageHandler {
 public:
  virtual ~MessageHandler() {}
  virtual void OnMessage(void*) = 0;

 protected:
  MessageHandler() {}

 private:
  MessageHandler(const MessageHandler&) = delete;
  MessageHandler& operator=(const MessageHandler&) = delete;
};

class DummyMessageHandler : public MessageHandler {
 public:
  virtual void OnMessage(void*) {}
};

class InvokeFunctorMessageHandler : public MessageHandler {
 public:
  explicit InvokeFunctorMessageHandler(FunctionView<void()> functor)
      : functor_(functor) {}

  ~InvokeFunctorMessageHandler() {}

  void OnMessage(void*) override {
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

template <class FunctorT>
class PostFunctorMessageHandler : public MessageHandler {
 public:
  explicit PostFunctorMessageHandler(FunctorT&& functor)
      : functor_(std::forward<FunctorT>(functor)) {}

  void OnMessage(void*) override { functor_(); }

 private:
  ~PostFunctorMessageHandler() override {}
  typename std::remove_reference<FunctorT>::type functor_;
};

class Executor {
 public:
  Executor();

  ~Executor();

  bool Start();

  void Stop();

  template <
      class ReturnT,
      typename = typename std::enable_if<!std::is_void<ReturnT>::value>::type>
  ReturnT Invoke(FunctionView<ReturnT()> functor) {
    ReturnT result;
    InvokeInternal([functor, &result] { result = functor(); });
    return result;
  }

  template <
      class ReturnT,
      typename = typename std::enable_if<std::is_void<ReturnT>::value>::type>
  void Invoke(FunctionView<void()> functor) {
    InvokeInternal(functor);
  }

  template <class FunctorT>
  void PostTask(FunctorT&& functor) {
    MessageHandler* p = new PostFunctorMessageHandler<FunctorT>(
        std::forward<FunctorT>(functor));
    std::shared_ptr<MessageHandler> handler(p);
    queue_.Push(handler);
  }

 private:
  void Run();
  void Quit();
  void Restart();
  void WakeUp();
  bool IsRunning();
  bool IsQuitting();
  bool IsCurrent();
  void InvokeInternal(FunctionView<void()> functor);

 private:
  std::unique_ptr<std::thread> thread_;
  SafeQueue<std::shared_ptr<MessageHandler>> queue_;
  volatile std::atomic_bool stop_;
  static thread_local Executor* self_;
};

}  // namespace qrtc

#endif  // __EXECUTOR_H__
