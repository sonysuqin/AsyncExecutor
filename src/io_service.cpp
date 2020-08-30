#include "io_service.h"
#include "asio_timer.h"
#include "beast_websocket.h"
#include "boost/asio/io_context.hpp"

namespace boost {
namespace asio {
// This class is only used for hidding boost headers.
class io_service_work : public io_context::work {
 public:
  explicit io_service_work(boost::asio::io_context& io_context)
      : io_context::work(io_context) {}
  io_service_work(const io_service_work& other) : io_context::work(other) {}
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
    if (running_) {
      break;
    }

    // Create io_context.
    ioc_.reset(new boost::asio::io_context);
    // Create io_service_work to keep io_context running.
    work_.reset(new boost::asio::io_service_work(*ioc_));
    // Create thread for io_context run().
    thread_.reset(new std::thread([this] { ioc_->run(); }));
    // Set TLS for current thread flag.
    ioc_->post([this] { SetCurrent(); });
    // Now IOService is indeed running.
    running_ = true;
  } while (0);
  return ret;
}

bool IOService::Stop() {
  bool ret = true;
  do {
    if (!running_) {
      break;
    }

    // For getting rid of memory leak from openssl.
    InvokeInternal([this] { OPENSSL_thread_stop(); });

    // From this time on, no new invoke is allowed.
    running_ = false;

    // Stop run() loop, no run() will be called again.
    work_.reset();
    // Interrupt current run().
    ioc_->stop();

    // Join the thread until it stopped.
    if (thread_ != nullptr && !IsCurrent()) {
      thread_->join();
      thread_.reset();
    }

    // Delete ios.
    ioc_.reset();
  } while (0);
  return ret;
}

void IOService::SetCurrent() {
  self_ = this;
}

bool IOService::IsCurrent() {
  return self_ == this;
}

std::shared_ptr<Timer> IOService::CreateTimer() {
  if (!running_ || ioc_ == nullptr) {
    return nullptr;
  }
  return std::make_shared<AsioTimer>(*ioc_);
}

std::shared_ptr<WebSocket> IOService::CreateWebSocket() {
  if (!running_ || ioc_ == nullptr) {
    return nullptr;
  }
  return std::make_shared<BeastWebSocket>(*ioc_);
}

void IOService::InvokeInternal(FunctionView<void()> functor) {
  std::shared_ptr<FunctorInvoker> functor_wrapper(new FunctorInvoker(functor));
  bool posted = false;
  if (IsCurrent()) {
    functor_wrapper->run();
  } else {
    if (running_ && ioc_ != nullptr) {
      ioc_->post([functor_wrapper] { functor_wrapper->run(); });
      posted = true;
    }
  }
  if (running_ && posted) {
    functor_wrapper->wait();
  }
}

void IOService::PostInternal(std::shared_ptr<FunctorWrapper> functor_wrapper) {
  if (running_ && ioc_ != nullptr && functor_wrapper != nullptr) {
    ioc_->post([functor_wrapper] { functor_wrapper->run(); });
  }
}

}  // namespace bee
