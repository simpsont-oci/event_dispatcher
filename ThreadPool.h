#pragma once

#include "Barrier.h"

#include <thread>
#include <vector>

class ThreadPool {
public:
  ThreadPool() = delete;
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  template <typename Func>
  ThreadPool(size_t count, Func&& func) : threads_(), bar_(count + 1) {
    for (size_t i = 0; i < count; ++i) {
      threads_.emplace_back([&, fx = std::move(func)](){
        bar_.wait();
        fx();
        bar_.wait();
      });
    }
    bar_.wait();
  }
  ~ThreadPool();

private:
  std::vector<std::thread> threads_;
  Barrier bar_;
};

