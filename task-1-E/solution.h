//
//  solution.h
//  Tournament_tree_mutex
//
//  Created by Igashov_Ilya on 27.02.17.
//  Copyright (c) 2017 Igashov_Ilya. All rights reserved.
//

#include <atomic>
#include <thread>
#include <array>
#include <vector>

// Power function for integers.
size_t pow(size_t base, size_t degree)
{
    size_t result = 1;
    for (size_t i = 0; i < degree; ++i) {
        result *= base;
    }
    return result;
}

// Binary logarithm for integers.
// Works like ceil(log2(arg)) from <cmath>:
// if arg is a precise degree of 2 (variable flag indicates it) then the result will be precise too,
// else the result will be rounded up.
size_t log2(size_t arg) {
    bool flag = false;
    size_t base = 0;
    while (arg >> 1 != 0) {
        if ((arg & 1) != 0) {
            flag = true;
        }
        arg >>= 1;
        ++base;
    }
    return flag ? base + 1 : base;
}


class PetersonMutex {
public:
    PetersonMutex():
    victim_(0)
    {
        want_[0].store(false);
        want_[1].store(false);
    }
    
    void lock(int thread_id) {
        want_[thread_id].store(true);
        victim_.store(thread_id);
        while (want_[1 - thread_id].load() && victim_.load() == thread_id) {
            std::this_thread::yield();
        }
    }
    
    void unlock(int thread_id) {
        want_[thread_id].store(false);
    }
    
private:
    std::array<std::atomic<bool>, 2> want_;
    std::atomic<int> victim_;
};

class TreeMutex {
public:
    TreeMutex(size_t n_threads):
    leafs_num_(pow(2, log2(n_threads))),
    height_(log2(leafs_num_)),
    peterson_mtx_(leafs_num_ - 1)
    {
    }
    
    // In order to lock the TreeMutex, current_thread has to reach the root of the tree going through
    // Peterson mutexes - nodes on the path in the tree.
    void lock(size_t current_thread) {
        size_t node = leafs_num_ - 1 + current_thread;
        for (size_t level = 0; level < height_; ++level) {
            size_t mtx = (node - 1) / 2;
            bool peterson_mtx_th_id = node % 2;
            peterson_mtx_[mtx].lock(peterson_mtx_th_id);
            node = mtx;
        }
        locked_thread_.store(current_thread);
    }
    
    // We will unlock Peterson mutexes in reversed order (the last mutex we locked is always the first one
    // in our tree: it is a root). In this case we should rebuild the path from the leaf
    // containing current thread to root. Binary code of current_thread keeps all necessary information.
    // In every level we will take a digit from binary code and determine next Peterson mtx index and thread's ID
    // in this mutex.
    void unlock(size_t current_thread) {
        if (locked_thread_.load() == current_thread) {
            size_t mtx = 0;
            for (int level = height_ - 1; level >= 0; --level) {
                int digit = ((current_thread & (1 << level)) == 0) ? 0 : 1;
                bool peterson_mtx_th_id = 1 - digit;
                peterson_mtx_[mtx].unlock(peterson_mtx_th_id);
                mtx = mtx * 2 + pow(2, digit);
            }
            locked_thread_.store(leafs_num_);
        }
    }
    
    TreeMutex(const TreeMutex&) = delete;
    TreeMutex& operator=(const TreeMutex&) = delete;
    
private:
    size_t leafs_num_;
    size_t height_;
    std::vector<PetersonMutex> peterson_mtx_;
    std::atomic<size_t> locked_thread_;
};
