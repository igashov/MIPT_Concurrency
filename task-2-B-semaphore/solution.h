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
  Semaphore(const size_t start = 0) : signals_counter_(start) {}
  
  // Thread will wait untill there is at least one signal.
  void Wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (signals_counter_ == 0) {
      get_signal_cv_.wait(lock, [this](){ return signals_counter_ > 0; });
    }
    --signals_counter_;
  }
  
  // Sends another signal that means that signals_counter_ increments.
  void Signal() {
    std::unique_lock<std::mutex> lock(mtx_);
    ++signals_counter_;
    get_signal_cv_.notify_one();
  }
  
 private:
  size_t signals_counter_;
  std::condition_variable get_signal_cv_;
  std::mutex mtx_;
};

// Implememtation of Robot using semaphores.
class Robot {
 public:
  void StepLeft() {
    left_semaphore_.Wait();
    std::cout << "left" << std::endl;
    right_semaphore_.Signal();
  }
  
  void StepRight() {
    right_semaphore_.Wait();
    std::cout << "right" << std::endl;
    left_semaphore_.Signal();
  }
  
 private:
  Semaphore left_semaphore_{1};
  Semaphore right_semaphore_;
};