#ifndef __TIMER_H__
#define __TIMER_H__

#include <functional>
#include <random>

namespace bee {

class Timer {
 public:
  typedef std::function<void(void)> TimerCallback;
  Timer();
  virtual ~Timer(){};

  // Open and start a timer, if |repeat| is true, |callback| will be called
  // every |timeout| milliseconds, otherwise, |callback| will only be called
  // once.
  virtual void Open(int timeout, bool repeat, TimerCallback callback) = 0;

  // Close the timer.
  virtual void Close() = 0;

  // Return timer id.
  uint64_t GetId() { return id_; }

 private:
  static std::default_random_engine generator_;
  static std::uniform_int_distribution<uint64_t> distribution_;
  static uint64_t current_id_;
  uint64_t id_ = 0;
};

}  // namespace bee

#endif
