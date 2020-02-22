#include "executor.h"

namespace qrtc {

thread_local Executor* Executor::self_ = nullptr;

Executor::Executor() : stop_(false) {}

Executor::~Executor() {}

bool Executor::Start() {
  if (IsRunning()) {
    return false;
  }

  Restart();  // reset IsQuitting() if the thread is being restarted
  thread_.reset(new std::thread(&Executor::Run, this));
  return true;
}

void Executor::Stop() {
  if (!IsRunning()) {
    return;
  }

  Quit();
  thread_->join();
  thread_.reset();
}

void Executor::Run() {
  self_ = this;
  while (!IsQuitting()) {
    std::shared_ptr<MessageHandler> handler;
    if (queue_.BlockPop(handler) && handler != nullptr) {
      handler->OnMessage(NULL);
    }
  }
}

void Executor::Quit() {
  stop_ = true;
  WakeUp();
}

void Executor::Restart() {
  stop_ = false;
}

void Executor::WakeUp() {
  std::shared_ptr<DummyMessageHandler> handler(new DummyMessageHandler());
  queue_.Push(handler);
}

bool Executor::IsRunning() {
  return thread_ != nullptr;
}

bool Executor::IsQuitting() {
  return stop_;
}

bool Executor::IsCurrent() {
  return self_ == this;
}

void Executor::InvokeInternal(FunctionView<void()> functor) {
  std::shared_ptr<InvokeFunctorMessageHandler> handler(
      new InvokeFunctorMessageHandler(functor));
  if (IsCurrent()) {
    handler->OnMessage(NULL);
  } else {
    queue_.Push(handler);
  }
  handler->wait();
}

}  // namespace qrtc
