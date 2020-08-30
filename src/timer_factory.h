#ifndef __TIMER_FACTORY_H__
#define __TIMER_FACTORY_H__

#include <memory>

#include "timer.h"

namespace bee {

class TimerFactory {
 public:
  virtual std::shared_ptr<Timer> CreateTimer() = 0;
};

}  // namespace bee

#endif  // __TIMER_FACTORY_H__
