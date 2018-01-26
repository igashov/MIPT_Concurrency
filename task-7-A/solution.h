//
//  solution.h
//  Lock_free_stack
//
//  Created by Igashov_Ilya on 06.05.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include <atomic>

///////////////////////////////////////////////////////////////////////

template <typename T>
class LockFreeStack {
  struct Node {
    T element;
    std::atomic<Node*> next{nullptr}; // Atomic is to prevent race.
    
    explicit Node(T value) : element(value) {}
  };
  
 public:
  explicit LockFreeStack() {
  }
  
  ~LockFreeStack() {
    while (garbage_top_.load() != nullptr) {
      Node* tmp = garbage_top_.load()->next.load();
      delete garbage_top_.load();
      garbage_top_.store(tmp);
    }
    
    while (stack_top_.load() != nullptr) {
      Node* tmp = stack_top_.load()->next.load();
      delete stack_top_.load();
      stack_top_.store(tmp);
    }
  }
  
  void Push(T element) {
    Node* new_node = new Node(element);
    Node* old_stack_top = stack_top_.load();
    do {
      new_node->next.store(old_stack_top);
    } while (!stack_top_.compare_exchange_strong(old_stack_top, new_node));
  }
  
  bool Pop(T& element) {
    Node* old_stack_top = stack_top_.load();
    do {
      if (old_stack_top == nullptr) {
        return false;
      }
    } while (!stack_top_.compare_exchange_strong(old_stack_top, old_stack_top->next.load()));
    
    // Add element to garbage list.
    Node* old_garbage_top = garbage_top_.load();
    do {
      old_stack_top->next.store(old_garbage_top);
    } while (!garbage_top_.compare_exchange_strong(old_garbage_top, old_stack_top));
    
    element = old_stack_top->element;
    return true;
  }
  
 private:
  std::atomic<Node*> stack_top_{nullptr};
  std::atomic<Node*> garbage_top_{nullptr};
};

/////////////////////////////////////////////////////////////////////

template <typename T>
using ConcurrentStack = LockFreeStack<T>;