#pragma once

#include <condition_variable>
#include <mutex>

class Barrier {
public:
  Barrier(size_t);
  void wait();

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  size_t full_count_;
  size_t count_;
};


