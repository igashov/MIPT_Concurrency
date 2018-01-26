//
//  solution.h
//  Lock_free_queue
//
//  Created by Igashov_Ilya on 11.05.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include <thread>
#include <atomic>

///////////////////////////////////////////////////////////////////////

template <typename T, template <typename U> class Atomic = std::atomic>
class LockFreeQueue {
  struct Node {
    T element_{};
    Atomic<Node*> next_{nullptr};
    
    explicit Node(T element, Node* next = nullptr)
        : element_(std::move(element)),
          next_(next) {}
    
    explicit Node() {}
  };
  
 public:
  explicit LockFreeQueue() {
    Node* dummy = new Node{};
    head_ = dummy;
    tail_ = dummy;
    start_ = dummy;
  }
  
  ~LockFreeQueue() {
    // We free all nodes in the list from start_
    // (it is guaranteed, that all nodes before the start_ were freed earlier).
    while (start_ != nullptr) {
      Node* tmp = start_->next_.load();
      delete start_;
      start_ = tmp;
    }
  }
  
  void Enqueue(T element) {
    Node* new_tail = new Node(element);
    Node* curr_tail = nullptr;
    while (true) {
      // Firstly we memorize current tail.
      curr_tail = tail_.load();
      // Then we check pointer to the next node after current tail.
      if (!curr_tail->next_.load()) {
        // If next node is null, that's alright =>
        // we add element and leave the cycle.
        Node* tmp = nullptr;
        if (curr_tail->next_.compare_exchange_strong(tmp, new_tail)) {
          break;
        }
        // If next node is not null, then something went wrong:
        // either tail_ is wrong (and we try to fix it)
        // or tail_ just has changed (so we try to fix curr_tail).
        // Anyway, we keep rolling in the cycle.
      } else {
        tail_.compare_exchange_strong(curr_tail, curr_tail->next_.load());
      }
    }
    // Finally we should fix pointer to the tail of the queue.
    tail_.compare_exchange_strong(curr_tail, new_tail);
  }
  
  bool Dequeue(T& element) {
    // Count calls of Dequeue for garbage collection.
    counter_.fetch_add(1);
    
    Node* curr_head = nullptr;
    Node* curr_tail = nullptr;
    while (true) {
      // Firstly we memorize current head and tail.
      curr_head = head_.load();
      curr_tail = tail_.load();
      if (curr_head == curr_tail) {
        // If curr_head and curr_tail conicided  and the next node is null,
        // then the queue is empty (it consists of the dummy node only).
        if (!curr_head->next_.load()) {
          return false;
          // If the next node is not null, then
          // either another thread has added a node but hasn't fixed ptr to tail yet
          // or tail_ just has changed.
          // Anyway, we keep rolling in the cycle.
        } else {
          tail_.compare_exchange_strong(curr_head, curr_head->next_.load());
        }
      } else {
        // If curr_head and curr_tail don't conicide and head_ hasn't changed yet,
        // that's alright.
        if (head_.compare_exchange_strong(curr_head, curr_head->next_.load())) {
          element = curr_head->next_.load()->element_;
          return true;
        }
      }
    }
    
    // Finally we try to free unused memory.
    // We do it if we caught a quiescent period (at the moment this is the only Dequeue call).
    if (counter_.fetch_sub(1) == 1) {
      // We free all nodes in the list from start_ to head_.
      while (start_ != head_.load()) {
        Node* tmp = start_->next_.load();
        delete start_;
        start_ = tmp;
      }
    }
  }
  
 private:
  Atomic<Node*> head_{nullptr};
  Atomic<Node*> tail_{nullptr};
  Node* start_{nullptr};
  Atomic<size_t> counter_{0};
};

///////////////////////////////////////////////////////////////////////