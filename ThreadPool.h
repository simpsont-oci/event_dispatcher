#pragma once

#include "Barrier.h"

#include <functional>
#include <thread>
#include <utility>
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
      threads_.emplace_back(
        std::bind(
          [&](Func& fx){
            bar_.wait();
            fx();
            bar_.wait();
          },
          std::move(func)
        )
      );
    }
    bar_.wait();
  }
  ~ThreadPool();

private:
  std::vector<std::thread> threads_;
  Barrier bar_;
};

