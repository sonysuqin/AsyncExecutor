#ifndef __TIMER_H__
#define __TIMER_H__

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

#endif  // __TIMER_H__
