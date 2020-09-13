// Tencent is pleased to support the open source community by making Mars
// available. Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights
// reserved.

// Licensed under the MIT License (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License
// at http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" basis, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef CONDITION_H_
#define CONDITION_H_

#include <errno.h>
#include <climits>
#include <condition_variable>
#include <mutex>

#include "xlog/comm/assert/__assert.h"
#include "xlog/comm/thread/atomic_oper.h"

class Condition {
 public:
  Condition() : anyway_notify_(0) {}

  ~Condition() {}

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!atomic_cas32(&anyway_notify_, 0, 1))
      condition_.wait(lock);

    anyway_notify_ = 0;
  }

  int wait(unsigned long millisecond) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool ret = false;
    if (!atomic_cas32(&anyway_notify_, 0, 1))
      ret = condition_.wait_for(lock, std::chrono::milliseconds(millisecond)) !=
            std::cv_status::timeout;

    anyway_notify_ = 0;

    if (!ret)
      return ETIMEDOUT;

    return 0;
  }

  void notifyOne() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.notify_one();
  }

  void notifyAll(bool anywaynotify = false) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (anywaynotify)
      anyway_notify_ = 1;

    condition_.notify_all();
  }

  void cancelAnyWayNotify() { anyway_notify_ = 0; }

 private:
  Condition(const Condition&);
  Condition& operator=(const Condition&);

 private:
  std::condition_variable condition_;
  std::mutex mutex_;
  volatile unsigned int anyway_notify_;
};

#endif
