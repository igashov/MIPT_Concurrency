//
//  solution.h
//  Robot_semaphore
//
//  Created by Igashov_Ilya on 22.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>

// Implementation of semaphore.
class Semaphore {
 public:
  Semaphore(): signals_counter_(0) {}
  
  void Wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (signals_counter_.load() == 0) {
      get_signal_cv_.wait(lock, [this](){ return signals_counter_ > 0;});
    }
    signals_counter_.fetch_sub(1);
  }
  
  void Signal() {
    std::unique_lock<std::mutex> lock(mtx_);
    signals_counter_.fetch_add(1);
    get_signal_cv_.notify_all();
  }
  
 private:
  std::atomic<size_t> signals_counter_;
  std::condition_variable get_signal_cv_;
  std::mutex mtx_;
};

// Implememtation of ropot using semaphores.
class Robot {
 public:
  void StepLeft() {
    std::cout << "left" << std::endl;
    right_semaphore.Signal();
    left_semaphore_.Wait();
  }
  
  void StepRight() {
    right_semaphore.Wait();
    std::cout << "right" << std::endl;
    left_semaphore_.Signal();
  }
  
 private:
  Semaphore left_semaphore_;
  Semaphore right_semaphore;
};