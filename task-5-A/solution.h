//  solution.h
//  MCS_spinlock
//
//  Created by Igashov_Ilya on 12.04.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include "spinlock_pause.h"

#include <iostream>
#include <atomic>
#include <thread>

///////////////////////////////////////////////////////////////////////

// scalable and fair MCS (Mellor-Crummy, Scott) spinlock implementation
//
// usage:
// {
// MCSSpinLock::Guard guard(spinlock); // spinlock acquired
// ... // in critical section
// } // spinlock released
//

///////////////////////////////////////////////////////////////////////

template <template <typename T> class Atomic = std::atomic>
class MCSSpinLock {
 public:
  class Guard {
   public:
    explicit Guard(MCSSpinLock& spinlock) : spinlock_(spinlock) {
      Acquire();
    }
    
    ~Guard() {
      Release();
    }
    
   private:
    // Add self to spinlock queue and wait for ownership.
    void Acquire() {
      Guard* prev_tail = spinlock_.wait_queue_tail_.exchange(this);
      
      if (prev_tail != nullptr) {
        prev_tail->next_.store(this);
      } else {
        is_owner_.store(true);
      }
      
      while (!is_owner_.load()) {}
    }
    
    // Transfer ownership to the next guard node in spinlock wait queue
    // or reset tail pointer if there are no other contenders.
    void Release() {
      Guard* tmp = this;
      // Firstly we check if the next node is already linked.
      if (next_.load() != nullptr) {
        next_.load()->is_owner_.store(true);
      } else if (!spinlock_.wait_queue_tail_.compare_exchange_strong(tmp, nullptr)) {
        // Here we checked if a node exicts.
        // And if so, we are waiting for linking and then set its is_owner true.
        while(this->next_.load() == nullptr) {}
        this->next_.load()->is_owner_.store(true);
      }
    }
    
   private:
    MCSSpinLock& spinlock_;
    
    Atomic<bool> is_owner_{false};
    Atomic<Guard*> next_{nullptr};
  };
  
 private:
  Atomic<Guard*> wait_queue_tail_{nullptr};
};

/////////////////////////////////////////////////////////////////////

// alias for checker
template <template <typename T> class Atomic = std::atomic>
using SpinLock = MCSSpinLock<Atomic>;

/////////////////////////////////////////////////////////////////////
