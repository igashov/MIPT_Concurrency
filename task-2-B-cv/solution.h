//
//  solution.h
//  Robot_cv
//
//  Created by Igashov_Ilya on 22.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>

// Implememtation of Robot using condition variables only.
class Robot {
 public:
  Robot(): left_stepped_(false) {}
  
  void StepLeft() {
    std::unique_lock<std::mutex> lock(mtx_);
    std::cout << "left" << std::endl;
    left_stepped_ = true;
    another_leg_stepped_.notify_one();
    another_leg_stepped_.wait(lock, [this](){ return !left_stepped_; });
  }
  
  void StepRight() {
    std::unique_lock<std::mutex> lock(mtx_);
    another_leg_stepped_.wait(lock, [this](){ return left_stepped_; });
    left_stepped_ = false;
    std::cout << "right" << std::endl;
    another_leg_stepped_.notify_one();
  }
  
 private:
  bool left_stepped_;
  std::condition_variable another_leg_stepped_;
  std::mutex mtx_;
};