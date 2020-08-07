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

IOService::~IOService() {}

bool IOService::Start() {
  bool ret = true;
  do {
    if (running_) {
      break;
    }

    ios_.reset(new boost::asio::io_service);
    work_.reset(new boost::asio::io_service_work(*ios_));
    thread_.reset(new std::thread([this] { ios_->run(); }));
    ios_->post([this] { SetCurrent(); });

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

    work_.reset();
    ios_->stop();

    if (thread_ != nullptr) {
      thread_->join();
      thread_.reset();
    }

    ios_.reset();
    running_ = false;
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
  if (ios_ == nullptr) {
    return nullptr;
  }
  return std::make_shared<AsioTimer>(*ios_);
}

void IOService::InvokeInternal(FunctionView<void()> functor) {
  std::shared_ptr<FunctorInvoker> functor_wrapper(new FunctorInvoker(functor));
  bool execute = true;
  if (IsCurrent()) {
    functor_wrapper->run();
  } else if (ios_ != nullptr) {
    ios_->post([functor_wrapper] { functor_wrapper->run(); });
  } else {
    execute = false;
  }
  if (execute) {
    functor_wrapper->wait();
  }
}

void IOService::PostInternal(std::shared_ptr<FunctorWrapper> functor_wrapper) {
  if (ios_ != nullptr) {
    ios_->post([functor_wrapper] { functor_wrapper->run(); });
  }
}

}  // namespace bee
