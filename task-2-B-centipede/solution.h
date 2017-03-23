//
//  solution.h
//  Robot_centipede
//
//  Created by Igashov_Ilya on 23.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>

// Implementation of semaphore.
class Semaphore {
 public:
  Semaphore(): signals_counter_(0) {}
  
  void wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (signals_counter_.load() == 0) {
      get_signal_cv_.wait(lock, [this](){ return signals_counter_ > 0;});
    }
    signals_counter_.fetch_sub(1);
  }
  
  void signal() {
    std::unique_lock<std::mutex> lock(mtx_);
    signals_counter_.fetch_add(1);
    get_signal_cv_.notify_all();
  }
  
 private:
  std::atomic<size_t> signals_counter_;
  std::condition_variable get_signal_cv_;
  std::mutex mtx_;
};

// Implementation of Robot-centipede using semaphores.
class Robot {
 public:
  explicit Robot(const std::size_t num_foots)
      : semaphores_(num_foots) {}
  
  // At start all legs are waiting for signal.
  // The last leg signals that the first leg can make a step.
  // First leg steps and signals the next one that it can step.
  // And so on.
  void Step(const std::size_t foot) {
    if (foot == semaphores_.size() - 1) {
      semaphores_[0].signal();
    }
    semaphores_[foot].wait();
    std::cout << "foot " << foot << std::endl;
    if (foot != semaphores_.size() - 1) {
      semaphores_[foot + 1].signal();
    }
  }
  
 private:
  std::vector<Semaphore> semaphores_;
};