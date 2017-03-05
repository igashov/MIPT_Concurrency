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
#include <cmath>

class PetersonMutex {
public:
    PetersonMutex() {
        want[0].store(false);
        want[1].store(false);
        victim.store(0);
    }
    
    void lock(int t) {
        want[t].store(true);
        victim.store(t);
        while (want[1 - t].load() && victim.load() == t) {
            std::this_thread::yield();
        }
    }
    
    void unlock(int t) {
        want[t].store(false);
    }
    
private:
    std::array<std::atomic<bool>, 2> want;
    std::atomic<int> victim;
};

class TreeMutex {
public:
    TreeMutex(size_t n_threads):
    leafs_num(pow(2, ceil(log2(n_threads)))),
    height(log2(leafs_num)),
    peterson_mtx(leafs_num - 1)
    {
    }
    
    // In order to lock the TreeMutex, current_thread has to reach the root of the tree going through
    // Peterson mutexes - nodes on the path in the tree.
    void lock(size_t current_thread) {
        size_t node = leafs_num - 1 + current_thread; // Index of the node that contains current thread.
        for (size_t level = 0; level < height; ++level) {
            size_t peterson_mtx_num = floor((node - 1) / 2); // Peterson mutex index.
            int peterson_mtx_th_id = node % 2; // Index of the current thread in the Peterson mutex (0 or 1).
            peterson_mtx[peterson_mtx_num].lock(peterson_mtx_th_id); // Current thread acquires Peterson mutex in this
            // level.
            node = peterson_mtx_num; // Since the thread locks in this level, assume we work with new thread with
            // id = peterson_mtx_num and it wants to lock in higher level.
        }
        locked_thread.store(current_thread);
    }
    
    // We will unlock Peterson mutexes in reversed order. In this case we should rebuild the path from the leaf
    // containing current thread to root. Binary code of current_thread keeps all necessary information.
    // In every level we will take a digit from binary code and determine next Peterson mtx index and thread's id
    // in this mutex.
    void unlock(size_t current_thread) {
        if (locked_thread.load() == current_thread) {
            int mtx = 0; // The last mutex we locked is always the first one in our tree (it is a root).
            for (int level = static_cast<int>(height - 1); level >= 0; --level) {
                int digit = ((current_thread & (1<<level)) == 0) ? 0 : 1; // Take a digit.
                int idx = 1 - digit; // Determine thread's index in the current Peterson mutex.
                peterson_mtx[mtx].unlock(idx); // Unlock this mtx.
                mtx = mtx * 2 + pow(2, digit); // Determine the index of the next mtx on our path.
            }
            locked_thread.store(leafs_num);
        }
    }
    
    TreeMutex(const TreeMutex&) = delete;
    TreeMutex& operator=(const TreeMutex&) = delete;
    
private:
    size_t leafs_num; // Number of leafs in the tree.
    size_t height; // Height of the tree.
    std::vector<PetersonMutex> peterson_mtx; // Non-leaf nodes of the tree are Peterson mutexes.
    std::atomic<size_t> locked_thread;
};

