//
//  solution.h
//  Lock_free_linked_set
//
//  Created by Igashov_Ilya on 19.05.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include "atomic_marked_pointer.h"
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

template <typename Element>
class LockFreeLinkedSet {
 private:
  struct Node {
    Element element_;
    AtomicMarkedPointer<Node> next_;
    
    Node(const Element& element, Node* next = nullptr)
        : element_{element},
          next_{next} {}
    
    bool Marked() const {
      return next_.Marked();
    }
    
    Node* NextPointer() const {
      return next_.LoadPointer();
    }
  };
  
  struct Edge {
    Node* pred_;
    Node* curr_;
    
    Edge(Node* pred, Node* curr)
        : pred_(pred),
          curr_(curr) {}
  };
  
 public:
  explicit LockFreeLinkedSet(ArenaAllocator& allocator)
      : allocator_(allocator),
        size_(0) {
    CreateEmptyList();
  }
  
  bool Insert(const Element& element) {
    Node* new_node = allocator_.New<Node>(element);
    while (true) {
      Edge edge = Locate(element);
      if (edge.curr_->element_ == element) {
        return false;
      }
      new_node->next_.Store(edge.curr_);
      if (edge.pred_->next_.CompareAndSet({edge.curr_, false}, {new_node, false})) {
        break;
      }
    }
    size_.fetch_add(1);
    return true;
  }
  
  bool Remove(const Element& element) {
    Edge edge{nullptr, nullptr};
    typename AtomicMarkedPointer<Node>::MarkedPointer curr_next{nullptr, false};
    while (true) {
      edge = Locate(element);
      if (edge.curr_->element_ != element) {
        return false;
      }
      
      curr_next = edge.curr_->next_.Load();
      if (!curr_next.marked_) {
        if (edge.curr_->next_.CompareAndSet(curr_next, {curr_next.ptr_, true})) {
          break;
        }
      }
    }
    if (!edge.pred_->next_.CompareAndSet({edge.curr_, false}, curr_next)) {
      edge = Locate(edge.curr_->element_);
    }
    size_.fetch_sub(1);
    return true;
  }
  
  bool Contains(const Element& element) {
    Edge edge = Locate(element);
    if (edge.curr_->element_ != element) {
      return false;
    } else {
      return true;
    }
  }
  
  size_t Size() const {
    return size_.load();
  }
  
 private:
  void CreateEmptyList() {
    head_ = allocator_.New<Node>(ElementTraits<Element>::Min());
    head_->next_.Store(allocator_.New<Node>(ElementTraits<Element>::Max()));
  }
  
  Edge Locate(const Element& element) {
    Node* left_node{nullptr};
    Node* left_node_next{nullptr};
    Node* right_node{nullptr};
    
    while (true) {
      Node* t = head_;
      
      // Find left node and right node.
      while (t->next_.Marked() || (t->element_ < element)) {
        if (!t->next_.Marked()) {
          left_node = t;
          left_node_next = t->next_.LoadPointer();
        }
        t = t->next_.LoadPointer();
      }
      right_node = t;
      
      // Check nodes are adjacent.
      if (left_node_next == right_node) {
        if (right_node->next_.Marked()) {
          continue;
        } else {
          return {left_node, right_node};
        }
      }
      
      // Remove marked elements.
      if (left_node->next_.CompareAndSet({left_node_next, false}, {right_node, false})) {
        if (right_node->next_.Marked()) {
          continue;
        } else {
          return {left_node, right_node};
        }
      }
    }
  }
  
private:
  ArenaAllocator& allocator_;
  Node* head_;
  std::atomic<size_t> size_;
};

///////////////////////////////////////////////////////////////////////

template <typename T> using ConcurrentSet = LockFreeLinkedSet<T>;

///////////////////////////////////////////////////////////////////////