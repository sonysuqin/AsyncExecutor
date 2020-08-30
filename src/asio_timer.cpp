#include "asio_timer.h"

namespace bee {

AsioTimer::AsioTimer(boost::asio::io_context& ios)
    : steady_timer_(ios), is_closed_(false), timeout_(0), repeat_(false) {}

AsioTimer::~AsioTimer() {}

void AsioTimer::Open(int32_t timeout, bool repeat, TimerCallback callback) {
  if (!is_closed_ && timeout > 0) {
    timeout_ = timeout;
    repeat_ = repeat;
    timer_callback_ = callback;

    steady_timer_.expires_from_now(std::chrono::milliseconds(timeout));
    steady_timer_.async_wait(
        std::bind(&AsioTimer::OnTimer, shared_from_this()));
  }
}

void AsioTimer::Close() {
  is_closed_ = true;
  boost::system::error_code ec;
  steady_timer_.cancel(ec);
}

void AsioTimer::OnTimer() {
  if (timer_callback_ && !is_closed_) {
    timer_callback_();

    if (repeat_) {
      steady_timer_.expires_from_now(std::chrono::milliseconds(timeout_));
      steady_timer_.async_wait(
          std::bind(&AsioTimer::OnTimer, shared_from_this()));
    }
  }
}

}  // namespace bee
