#include "io_service.h"
#include "asio_timer.h"

#include <boost/asio/io_service.hpp>

namespace boost {
namespace asio {
// This class is only used for hidding boost headers.
class io_service_work : public io_service::work {
 public:
  explicit io_service_work(boost::asio::io_context& io_context)
      : io_service::work(io_context) {}
  io_service_work(const io_service_work& other) : io_service::work(other) {}
};
}  // namespace asio
}  // namespace boost

namespace bee {

thread_local IOService* IOService::self_ = nullptr;

IOService::IOService() : running_(false) {}

IOService::~IOService() {
  Stop();
}

bool IOService::Start() {
  bool ret = true;
  do {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (running_) {
      break;
    }

    // Create io_service.
    ios_.reset(new boost::asio::io_service);
    // Create io_service_work to keep io_service running.
    work_.reset(new boost::asio::io_service_work(*ios_));
    // Create thread for io_service run().
    thread_.reset(new std::thread([this] { ios_->run(); }));
    // Set TLS for current thread flag.
    ios_->post([this] { SetCurrent(); });
    // Now IOService is indeed running.
    running_ = true;
  } while (0);
  return ret;
}

bool IOService::Stop() {
  bool ret = true;
  do {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!running_) {
      break;
    }

    // From this time on, no new invoke is allowed.
    running_ = false;

    // Stop run() loop, no run() will be called again.
    work_.reset();
    // Interrupt current run().
    ios_->stop();

    // Join the thread until it stopped.
    if (thread_ != nullptr && !IsCurrent()) {
      thread_->join();
      thread_.reset();
    }

    // Clear all timers before ios destroyed.
    if (!timer_table_.empty()) {
      for (auto timer : timer_table_) {
        timer.second->Close();
      }
      timer_table_.clear();
    }

    // Delete ios.
    ios_.reset();
  } while (0);
  return ret;
}

void IOService::SetCurrent() {
  self_ = this;
}

bool IOService::IsCurrent() {
  return self_ == this;
}

uint64_t IOService::OpenTimer(int timeout,
                              bool repeat,
                              Timer::TimerCallback callback) {
  uint64_t id = 0;
  InvokeInternal([this, timeout, repeat, callback, &id] {
    auto timer = std::make_shared<AsioTimer>(*ios_);
    id = timer->GetId();
    timer_table_[id] = timer;
    timer->Open(timeout, repeat, callback);
  });
  return id;
}

void IOService::CloseTimer(uint64_t id) {
  InvokeInternal([this, id] {
    auto iter = timer_table_.find(id);
    if (iter != timer_table_.end()) {
      iter->second->Close();
      timer_table_.erase(iter);
    }
  });
}

void IOService::InvokeInternal(FunctionView<void()> functor) {
  std::shared_ptr<FunctorInvoker> functor_wrapper(new FunctorInvoker(functor));
  bool posted = false;
  if (IsCurrent()) {
    functor_wrapper->run();
  } else {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (running_ && ios_ != nullptr) {
      ios_->post([functor_wrapper] { functor_wrapper->run(); });
      posted = true;
    }
  }
  if (running_ && posted) {
    functor_wrapper->wait();
  }
}

void IOService::PostInternal(std::shared_ptr<FunctorWrapper> functor_wrapper) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (running_ && ios_ != nullptr && functor_wrapper != nullptr) {
    ios_->post([functor_wrapper] { functor_wrapper->run(); });
  }
}

}  // namespace bee
