//
//  solution.h
//  Thread_pool
//
//  Created by Igashov_Ilya on 23.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <limits.h>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

// Blocking Queue that works with several threads.
template <class T, class Container = std::deque<T>>
class BlockingQueue {
 public:
  explicit BlockingQueue(const size_t& capacity)
      : capacity_(capacity),
        queue_is_shutted_(false) {}
  
  // Thread takes mutex and puts an element if queue is not full.
  // Otherwise, it frees mutex and waits untill either the queue shrinks,
  // or the queue is shutted down.
  // In the latter case it throws std::exception.
  void Put(T&& element) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (queue_.size() == capacity_) {
      queue_is_not_full_cv_.wait(lock);
    }
    if (queue_is_shutted_) {
      throw std::exception();
    }
    queue_.push_back(std::move(element));
    queue_is_not_empty_cv_.notify_all();
  }
  
  // Thread takes mutex and reads the last element if queue is not empty.
  // Otherwise, it frees mutex and waits untill either the queue grows,
  // or the queue is shutted down.
  // In the former case it writes value into result, pops and returns true.
  // In the latter case if queue is not empty it writes value into result
  // and returns true, else returns false.
  bool Get(T& result) {
    std::unique_lock<std::mutex> lock(mtx_);
    // We should leave the cycle even if queue is shutted, but size is 0 again.
    // Otherwise, we will not leave the cycle any more, because there will not
    // be any more notifications (queue is shutted).
    while (queue_.size() == 0 && !queue_is_shutted_) {
      queue_is_not_empty_cv_.wait(lock);
    }
    if (queue_is_shutted_ && queue_.size() == 0) {
      return false;
    }
    result = std::move(queue_.front());
    queue_.pop_front();
    // If queue is shutted, all threads are already notified.
    if (!queue_is_shutted_) {
      queue_is_not_full_cv_.notify_all();
    }
    return true;
  }
  
  // Forbids writing and notifies all threads.
  void Shutdown() {
    std::unique_lock<std::mutex> lock(mtx_);
    queue_is_shutted_ = true;
    queue_is_not_empty_cv_.notify_all();
    queue_is_not_full_cv_.notify_all();
  }
  
 private:
  Container queue_;
  const size_t capacity_;
  bool queue_is_shutted_;
  std::condition_variable queue_is_not_empty_cv_;
  std::condition_variable queue_is_not_full_cv_;
  std::mutex mtx_;
};

// Thread pool that distributes tasks between several threads.
template <class T>
class ThreadPool {
 public:
  ThreadPool() : tasks_(INT_MAX), shutted_(false) {
    for (size_t i = 0; i < default_num_workers(); ++i) {
      workers_.emplace_back(thread_initialization, this);
    }
  }
  
  explicit ThreadPool(const size_t num_threads)
      : tasks_(INT_MAX),
        shutted_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back(thread_initialization, this);
    }
  }
  
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  
  ~ThreadPool() {
    if (!shutted_) {
      Shutdown();
    }
  }
  
  std::future<T> Submit(std::function<T()> task) {
    std::packaged_task<T()> p_task(task);
    std::future<T> future = p_task.get_future();
    if (shutted_) {
      throw std::exception();
    }
    tasks_.Put(std::move(p_task));
    return future;
  }
  
  void Shutdown() {
    shutted_ = true;
    tasks_.Shutdown();
    for(std::thread &worker: workers_) {
      worker.join();
    }
  }
  
 private:
  size_t default_num_workers() {
    return std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 10;
  }
  
  static void thread_initialization(ThreadPool* me) {
    std::packaged_task<T()> task;
    while (me->tasks_.Get(task)) {
      task();
    }
  }
  
  BlockingQueue<std::packaged_task<T()>> tasks_;
  std::vector<std::thread> workers_;
  bool shutted_;
};
