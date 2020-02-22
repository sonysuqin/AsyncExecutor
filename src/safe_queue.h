#ifndef __SAFE_QUEUE_H__
#define __SAFE_QUEUE_H__

#include <mutex>
#include <queue>

#include "sem_wrap.h"

namespace qrtc {

template <class T>
class SafeQueue {
  enum { MAX_DEFAULT_SIZE = 16 * 1024, MAX_QUEUE_SIZE = 4000 * 1024 };

 public:
  SafeQueue(size_t queue_size = MAX_DEFAULT_SIZE);
  virtual ~SafeQueue();
  void Push(const T& value);
  void Push(T&& value);
  void BlockPush(const T& value);
  void BlockPush(T&& value);
  bool Pop(T& ret_value, int timeout);
  bool BlockPop(T& ret_value);
  size_t GetSize();
  bool IsEmpty();
  void Clear();

 protected:
  std::mutex mutex_;
  std::queue<T> queue_;
  size_t queue_size_;
  Semaphore empty_emaphore_;
  Semaphore full_semaphore_;
};

template <class T>
SafeQueue<T>::SafeQueue(size_t queue_size)
    : queue_size_(queue_size > MAX_QUEUE_SIZE ? MAX_QUEUE_SIZE : queue_size),
      empty_emaphore_(queue_size_, queue_size_),
      full_semaphore_(0, queue_size_) {}

template <class T>
SafeQueue<T>::~SafeQueue() {}

template <class T>
void SafeQueue<T>::Push(const T& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (queue_.size() >= queue_size_) {
    queue_.pop();
    full_semaphore_.Wait();
    empty_emaphore_.Post();
  }

  empty_emaphore_.Wait();
  queue_.push(value);
  full_semaphore_.Post();
}

template <class T>
void SafeQueue<T>::Push(T&& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (queue_.size() >= queue_size_) {
    queue_.pop();
    full_semaphore_.Wait();
    empty_emaphore_.Post();
  }

  empty_emaphore_.Wait();
  queue_.push(std::move(value));
  full_semaphore_.Post();
}

template <class T>
void SafeQueue<T>::BlockPush(const T& value) {
  empty_emaphore_.Wait();
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.push(value);
  full_semaphore_.Post();
}

template <class T>
void SafeQueue<T>::BlockPush(T&& value) {
  empty_emaphore_.Wait();
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.push(std::move(value));
  full_semaphore_.Post();
}

template <class T>
bool SafeQueue<T>::Pop(T& ret_value, int timeout) {
  if (full_semaphore_.Wait(timeout)) {
    std::lock_guard<std::mutex> lock(mutex_);
    ret_value = queue_.front();
    queue_.pop();
    empty_emaphore_.Post();
    return true;
  }
  return false;
}

template <class T>
bool SafeQueue<T>::BlockPop(T& ret_value) {
  full_semaphore_.Wait();
  std::lock_guard<std::mutex> lock(mutex_);
  ret_value = queue_.front();
  queue_.pop();
  empty_emaphore_.Post();
  return true;
}

template <class T>
size_t SafeQueue<T>::GetSize() {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

template <class T>
bool SafeQueue<T>::IsEmpty() {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.empty();
}

template <class T>
void SafeQueue<T>::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  size_t count = queue_.size();
  while (count--) {
    full_semaphore_.Wait();
    empty_emaphore_.Post();
  }
  swap(queue_, std::queue<T>());
}

}  // namespace qrtc

#endif  // __SAFE_QUEUE_H__
