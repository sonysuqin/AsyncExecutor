#ifndef __SEM_WRAP_H__
#define __SEM_WRAP_H__

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(IOS)
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

namespace qrtc {

class Semaphore {
 public:
  Semaphore(unsigned long init_count, unsigned long max_count);
  ~Semaphore();
  void Post();
  bool Wait();
  bool Wait(int timeout);  // ms

 private:
#ifdef WIN32
  HANDLE semaphore_;
#elif defined(IOS)
  dispatch_semaphore_t semaphore_;
#else
  sem_t semaphore_;
#endif
};

inline Semaphore::Semaphore(unsigned long init_count, unsigned long max_count) {
#ifdef WIN32
  semaphore_ = CreateSemaphore(NULL, init_count, max_count, NULL);
#elif defined(IOS)
  semaphore_ = dispatch_semaphore_create(init_count);
#else
  sem_init(&semaphore_, 0, init_count);
#endif
}

inline Semaphore::~Semaphore() {
#ifdef WIN32
  CloseHandle(semaphore_);
  semaphore_ = NULL;
#elif defined(IOS)
  // dispatch_release(semaphore_);  // Using arc.
#else
  sem_destroy(&semaphore_);
#endif
}

inline void Semaphore::Post() {
#ifdef WIN32
  ReleaseSemaphore(semaphore_, 1, NULL);
#elif defined(IOS)
  dispatch_semaphore_signal(semaphore_);
#else
  sem_post(&semaphore_);
#endif
}

inline bool Semaphore::Wait() {
#ifdef WIN32
  return (WAIT_OBJECT_0 == WaitForSingleObject(semaphore_, INFINITE));
#elif defined(IOS)
  return (0 == dispatch_semaphore_wait(semaphore_, DISPATCH_TIME_FOREVER));
#else
  return (0 == sem_wait(&semaphore_));
#endif
}

inline bool Semaphore::Wait(int timeout) {
#ifdef WIN32
  return (WAIT_OBJECT_0 == ::WaitForSingleObject(semaphore_, timeout));
#elif defined(IOS)
  return (0 == dispatch_semaphore_wait(
                   semaphore_,
                   dispatch_time(DISPATCH_TIME_NOW, timeout * NSEC_PER_MSEC)));
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  int timeout_sec = timeout / 1000;
  int timeout_ms = timeout % 1000;
  ts.tv_sec += timeout_sec;
  int ts_ms = ts.tv_nsec / 1000000;
  if (ts_ms + timeout_ms < 1000) {
    ts.tv_nsec += timeout_ms * 1000000;
  } else {
    ts.tv_sec += 1;
    ts.tv_nsec = (ts_ms + timeout_ms - 1000) * 1000000;
  }
  return (0 == sem_timedwait(&semaphore_, &ts));
#endif
}

}  // namespace qrtc

#endif  // __SEM_WRAP_H__
