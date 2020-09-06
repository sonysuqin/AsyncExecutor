#ifndef BEE_TIMER_H
#define BEE_TIMER_H

#include <functional>

namespace bee {

class Timer {
 public:
  typedef std::function<void(void)> TimerCallback;
  Timer() = default;
  virtual ~Timer() = default;

  // Open and start a timer, if |repeat| is true, |callback| will be called
  // every |timeout| milliseconds, otherwise, |callback| will only be called
  // once.
  virtual void Open(int32_t timeout, bool repeat, TimerCallback callback) = 0;

  // Close the timer.
  virtual void Close() = 0;
};

}  // namespace bee

#endif  // BEE_TIMER_H
