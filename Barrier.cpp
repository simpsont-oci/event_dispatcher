#include "Barrier.h"

Barrier::Barrier(size_t count) : mutex_(), cv_(), full_count_(count), count_(count) {
}

void Barrier::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (--count_ == 0) {
    cv_.notify_all();
    count_ = full_count_;
  } else {
    cv_.wait(lock);
  }
}

