//
//  solution.h
//  Cyclic_barrier
//
//  Created by Igashov_Ilya on 16.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

// Implementation of cyclic barrier class.
template <class ConditionVariable = std::condition_variable>
class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t num_threads)
      : n_threads_(num_threads),
        counter_of_waiting_threads_(2),
        current_epoche_(0) {}
  
  // If thread calls this method, it will wait untill all other threads call
  // this method too and only then this thread (as well as others) will continue.
  void Pass() {
    const int my = current_epoche_.load();
    if (counter_of_waiting_threads_[my].fetch_add(1) < n_threads_ - 1) {
      std::unique_lock<std::mutex> lock(mtx_);
      // Condition in cond_var_.wait args protects from spurious wakeups.
      all_threads_arrived_cv_.wait(lock, [this, &my](){ return counter_of_waiting_threads_[my].load() == n_threads_; });
    } else {
      // Switch the current epoche over to the next:
      // XOR for current_epoche_ variable, e.g. if current epoche has number 1,
      // the next epoche will have number 0.
      current_epoche_.store(current_epoche_.load() ^ 1);
      counter_of_waiting_threads_[current_epoche_.load()].store(0);
      all_threads_arrived_cv_.notify_all();
    }
  }
  
  CyclicBarrier(const CyclicBarrier&) = delete;
  CyclicBarrier& operator=(const CyclicBarrier&) = delete;
  
 private:
  size_t n_threads_;
  std::vector<std::atomic<size_t>> counter_of_waiting_threads_;
  std::atomic<int> current_epoche_;
  ConditionVariable all_threads_arrived_cv_;
  std::mutex mtx_;
};
