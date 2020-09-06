#ifndef BEE_TIMER_FACTORY_H
#define BEE_TIMER_FACTORY_H

#include <memory>

#include "timer.h"

namespace bee {

class TimerFactory {
 public:
  virtual std::shared_ptr<Timer> CreateTimer() = 0;
};

}  // namespace bee

#endif  // BEE_TIMER_FACTORY_H
