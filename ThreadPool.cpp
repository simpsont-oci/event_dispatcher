#include "ThreadPool.h"

ThreadPool::~ThreadPool() {
  bar_.wait();
  for (auto it = threads_.begin(); it != threads_.end(); ++it) {
    it->join();
  }
}

