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

//  Implementation of cyclic barrier class.
template <class ConditionVariable = std::condition_variable>
class CyclicBarrier {
public:
  explicit CyclicBarrier(size_t num_threads)
  : n_threads_(num_threads),
    counter_(2),
    current_(0) {}
  
  void Pass() {
    // Fix current epoche.
    int my = current_.load();
    // If there are not all threads are waiting yet in this epoche
    if (counter_[my].fetch_add(1) < n_threads_ - 1) {
      // Lock and wait until all threads won't reach the barrier in this epoche.
      // Condition in cond_var_.wait args protects from spurious wakeups.
      std::unique_lock<std::mutex> lock(mtx_);
      cond_var_.wait(lock, [&](){ return counter_[my].load() == n_threads_; });
    }
    // If this thread is the last and other threads have already reached the barrier in current epoche,
    // - switch the epoche over to the next
    //   (XOR for current_ variable, e.g. if current epoche has number 1, the next epoche will have number 0)
    // - set to zero the counter of threads in the epoche we swiched over to
    // - unblock all threads
    else {
      current_.store(current_.load() ^ 1);
      counter_[current_.load()].store(0);
      cond_var_.notify_all();
    }
  }
  
  CyclicBarrier(const CyclicBarrier&) = delete;
  CyclicBarrier& operator=(const CyclicBarrier&) = delete;
  
private:
  size_t n_threads_;
  //  counter_ is a vector of two counters that count waiting threads. One of these counters
  //  is responsible for current epoche, another one - for the next epoche (it is neseccary for
  //  cyclic property).
  std::vector<std::atomic<size_t>> counter_;
  //  current_ is current epoche indication - 0 or 1 - used for working with counter_ vector.
  std::atomic<int> current_;
  ConditionVariable cond_var_;
  std::mutex mtx_;
};
