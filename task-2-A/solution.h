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

template <class ConditionVariable = std::condition_variable>
class CyclicBarrier {
public:
  CyclicBarrier(size_t num_threads)
    : n_threads(num_threads),
      cnt_waits(0),
      is_not_waiting(false) {}
  
  void Pass() {
    if (cnt_waits.fetch_add(1) < n_threads - 1) {
      std::unique_lock<std::mutex> lock(mtx);
      //cond_var.wait(lock, [&]{return is_not_waiting;});
      cond_var.wait(lock);
    }
    else {
      is_not_waiting = true;
      cond_var.notify_all();
      cnt_waits.store(0);
    }
  }
  
  CyclicBarrier(const CyclicBarrier&) = delete;
  CyclicBarrier& operator=(const CyclicBarrier&) = delete;
  
private:
  int n_threads;
  std::atomic<int> cnt_waits;
  bool is_not_waiting;
  ConditionVariable cond_var;
  std::mutex mtx;
};
