#ifndef __ASIO_TIMER_H__
#define __ASIO_TIMER_H__

#include "boost/asio/io_service.hpp"
#include "boost/asio/steady_timer.hpp"
#include "timer.h"

#include <atomic>
#include <memory>

namespace bee {

// Timer based on asio steady timer, MUST be closed before IOService stopped.
class AsioTimer : public Timer, public std::enable_shared_from_this<AsioTimer> {
 public:
  AsioTimer(boost::asio::io_service& ios);
  ~AsioTimer() override;

 public:
  void Open(int timeout, bool repeat, TimerCallback callback) override;
  void Close() override;

 private:
  void OnTimer();

 private:
  boost::asio::steady_timer steady_timer_;
  std::atomic<bool> is_closed_;
  int32_t timeout_;
  bool repeat_;
  TimerCallback timer_callback_;
};

}  // namespace bee

#endif  // __ASIO_TIMER_H__
