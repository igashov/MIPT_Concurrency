//
//  solution.h
//  Striped_hash_set
//
//  Created by Igashov_Ilya on 31.03.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#pragma once

#include <algorithm>
#include <atomic>
#include <forward_list>
#include <functional>
#include <mutex>
#include <vector>

template <typename T, class Hash = std::hash<T>>
class StripedHashSet {
 public:
  explicit StripedHashSet(const size_t concurrency_level,
                          const size_t growth_factor = 3,
                          const double max_load_factor = 0.75)
      : size_(0),
        growth_factor_(growth_factor),
        max_load_factor_(max_load_factor),
        buckets_(20),
        stripes_(concurrency_level) {}
  
  // In order to insert an element, we should lock its stripe and only then work with its bucket.
  // After insertion the table may need extension:
  // in this case, we free the stripe and call Extend() - the method that extends the table.
  bool Insert(const T& element) {
    const size_t hash_value = hash_(element);
    std::unique_lock<std::mutex> lock(stripes_[GetStripeIndex(hash_value)]);
    size_t idx = GetBucketIndex(hash_value);
    if (std::find(buckets_[idx].begin(), buckets_[idx].end(), element) != buckets_[idx].end()) {
      return false;
    } else {
      buckets_[idx].emplace_front(element);
      size_.fetch_add(1);
      if (size_.load() / buckets_.size() > max_load_factor_) {
        lock.unlock();
        Extend();
      }
      return true;
    }
  }
  
  // In order to remove an element, we should lock its stripe and only then work with its bucket.
  bool Remove(const T& element) {
    const size_t hash_value = hash_(element);
    std::unique_lock<std::mutex> lock(stripes_[GetStripeIndex(hash_value)]);
    size_t idx = GetBucketIndex(hash_value);
    if (std::find(buckets_[idx].begin(), buckets_[idx].end(), element) != buckets_[idx].end()) {
      buckets_[idx].remove(element);
      size_.fetch_sub(1);
      return true;
    } else {
      return false;
    }
  }
  
  // Firstly we should lock the necessary stripe and only then work with the bucket
  // that potentially contains the element.
  bool Contains(const T& element) {
    const size_t hash_value = hash_(element);
    std::unique_lock<std::mutex> lock(stripes_[GetStripeIndex(hash_value)]);
    size_t idx = GetBucketIndex(hash_value);
    return std::find(buckets_[idx].begin(), buckets_[idx].end(), element) != buckets_[idx].end();
  }
  
  size_t Size() const {
    return size_.load();
  }
  
 private:
  size_t GetBucketIndex(const size_t hash_value) {
    return hash_value % buckets_.size();
  }
  
  size_t GetStripeIndex(const size_t hash_value) {
    return hash_value % stripes_.size();
  }
  
  // In order to extend the table, we should lock all stripes,
  // but at the first we need to check if anyone has already extended the table.
  void Extend() {
    std::vector<std::unique_lock<std::mutex>> locks;
    // It is enough to lock only the first stripe in order to check
    // if anyone has already extended the table.
    locks.emplace_back(stripes_[0]);
    if (size_.load() / buckets_.size() > max_load_factor_) {
      // We should lock stripes in order (e.g., from first one to the last one),
      // otherwise a deadlock may happen.
      for (size_t i = 1; i < stripes_.size(); ++i) {
        locks.emplace_back(stripes_[i]);
      }
      // When all stripes are locked, we extend the table.
      std::vector<std::forward_list<T>> new_buckets(buckets_.size() * growth_factor_);
      for (auto& bucket: buckets_) {
        for (auto& element: bucket) {
          size_t hash_value = hash_(element);
          size_t b = hash_value % new_buckets.size();
          new_buckets[b].push_front(element);
        }
      }
      buckets_.swap(new_buckets);
    }
  }
  
  std::atomic<size_t> size_;
  const size_t growth_factor_;
  const double max_load_factor_;
  std::vector<std::forward_list<T>> buckets_;
  std::vector<std::mutex> stripes_;
  Hash hash_;
};

template <typename T> using ConcurrentSet = StripedHashSet<T>;
