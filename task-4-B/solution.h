//
//  solution.h
//  Concurrent_optimistic_linked_set
//
//  Created by Igashov_Ilya on 01.04.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include "arena_allocator.h"

#include <atomic>
#include <limits>

///////////////////////////////////////////////////////////////////////

template <typename T>
struct ElementTraits {
  static T Min() {
    return std::numeric_limits<T>::min();
  }
  static T Max() {
    return std::numeric_limits<T>::max();
  }
};

///////////////////////////////////////////////////////////////////////

// Simple TaS spinlock.
class SpinLock {
 public:
  explicit SpinLock() {}
  
  void Lock() {
    while (locked_.test_and_set()) {}
  }
  
  void Unlock() {
    locked_.clear();
  }
  
 private:
  std::atomic_flag locked_{};
};

///////////////////////////////////////////////////////////////////////

// Singly-linked Concurrent Sorted List with Optimstic Locking.
template <typename T>
class OptimisticLinkedSet {
 private:
  struct Node {
    T element_;
    std::atomic<Node*> next_;
    SpinLock lock_{};
    std::atomic<bool> marked_{false};
    
    Node(const T& element, Node* next = nullptr)
        : element_(element),
          next_(next) {}
  };
  
  struct Edge {
    Node* pred_;
    Node* curr_;
    
    Edge(Node* pred, Node* curr)
        : pred_(pred),
          curr_(curr) {}
  };
  
 public:
  explicit OptimisticLinkedSet(ArenaAllocator& allocator) : allocator_(allocator) {
    CreateEmptyList();
  }
  
  // In order to insert an element, we should:
  // - find necessary edge (Locate)
  // - lock the first node in this edge (because we will change its next_)
  // - validate after locking (between locating and locking the situation might change)
  // - if validation failed, we should free locks and start all over again.
  // Only then we can insert an element.
  bool Insert(const T& element) {
    Edge edge{nullptr, nullptr};
    bool valid = false;
    do {
      edge = Locate(element);
      edge.pred_->lock_.Lock();
      valid = Validate(edge);
      if (!valid) {
        edge.pred_->lock_.Unlock();
      }
    } while (!valid);
    
    if (edge.curr_->element_ == element) {
      edge.pred_->lock_.Unlock();
      return false;
    } else {
      Node* inserted_element = allocator_.New<Node>(element);
      inserted_element->next_.store(edge.curr_);
      edge.pred_->next_.store(inserted_element);
      size_.fetch_add(1);
      edge.pred_->lock_.Unlock();
      return true;
    }
  }
  
  // In order to remove an element, we should:
  // - find necessary edge (Locate)
  // - lock this edge (because we will work with both nodes)
  // - validate after locking (between locating and locking the situation might change)
  // - if validation failed, we should free locks and start all over again.
  // Only then we can remove an element.
  bool Remove(const T& element) {
    Edge edge{nullptr, nullptr};
    bool valid = false;
    do {
      edge = Locate(element);
      edge.pred_->lock_.Lock();
      edge.curr_->lock_.Lock();
      valid = Validate(edge);
      if (!valid) {
        edge.pred_->lock_.Unlock();
        edge.curr_->lock_.Unlock();
      }
    } while (!valid);
    
    if (edge.curr_->element_ != element) {
      edge.pred_->lock_.Unlock();
      edge.curr_->lock_.Unlock();
      return false;
    } else {
      edge.curr_->marked_.store(true);
      edge.pred_->next_.store(edge.curr_->next_.load());
      size_.fetch_sub(1);
      edge.pred_->lock_.Unlock();
      edge.curr_->lock_.Unlock();
      return true;
    }
  }
  
  // Finds necessary edge and compares its second node with the element.
  bool Contains(const T& element) const {
    Edge edge = Locate(element);
    return edge.curr_->element_ == element;
  }
  
  size_t Size() const {
    return size_.load();
  }
  
 private:
  // Initially the list has two nodes: -inf and +inf.
  void CreateEmptyList() {
    head_ = allocator_.New<Node>(ElementTraits<T>::Min());
    head_->next_ = allocator_.New<Node>(ElementTraits<T>::Max());
  }
  
  // Searching for the necessary edge without locking any nodes.
  // Starting in the head, we are looking for two sequent nodes:
  // first of them (pred_) is less then the element and the next one is greater than or equal to the element.
  Edge Locate(const T& element) const {
    Edge current_edge{head_, head_->next_.load()};
    while (current_edge.curr_->element_ < element) {
      current_edge.pred_ = current_edge.curr_;
      current_edge.curr_ = current_edge.curr_->next_.load();
    }
    return current_edge;
  }
  
  // To validate the given edge means to check if edge's nodes are still sequent and their marks are still false.
  // Successful validation means that we are still working with correct nodes in the "master branch".
  bool Validate(const Edge& edge) const {
    if (edge.pred_->next_.load() == edge.curr_ && !edge.pred_->marked_.load() && !edge.curr_->marked_.load()) {
      return true;
    } else {
      return false;
    }
  }
  
 private:
  ArenaAllocator& allocator_;
  Node* head_{nullptr};
  std::atomic<size_t> size_{0};
};

template <typename T> using ConcurrentSet = OptimisticLinkedSet<T>;

///////////////////////////////////////////////////////////////////////
