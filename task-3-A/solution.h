//
//  solution.h
//  Blocking_queue
//
//  Created by Igashov_Ilya on 23.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>

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