/**
 * @file benchmark_buffer_pool.cpp
 * @brief 缓冲池性能基准测试
 */

#include "buffer_pool.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace std::chrono;

// 测试直接分配 vs 对象池
void benchmark_allocation() {
    const int iterations = 100000;
    volatile char dummy = 0;

    // 测试直接分配
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        auto buffer = std::make_unique<std::vector<char>>(4096);
        (*buffer)[0] = 'X';
        dummy = (*buffer)[0];  // 防止编译器优化
    }
    auto end = high_resolution_clock::now();
    auto direct_time = duration_cast<microseconds>(end - start).count();

    // 测试对象池
    auto& pool = BufferPool::instance();
    start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        auto buffer = pool.acquire();
        buffer->data[0] = 'X';
        dummy = buffer->data[0];  // 防止编译器优化
    }
    end = high_resolution_clock::now();
    auto pool_time = duration_cast<microseconds>(end - start).count();

    std::cout << "=== Allocation Benchmark (" << iterations << " iterations) ===" << std::endl;
    std::cout << "Direct allocation: " << direct_time << " μs" << std::endl;
    std::cout << "Object pool:       " << pool_time << " μs" << std::endl;
    std::cout << "Speedup:           " << (double)direct_time / pool_time << "x" << std::endl;
    std::cout << std::endl;
}

// 测试多线程性能
void benchmark_multithreaded() {
    const int num_threads = 8;
    const int ops_per_thread = 10000;

    auto& pool = BufferPool::instance();

    auto start = high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&pool, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; j++) {
                auto buffer = pool.acquire();
                buffer->data[0] = 'X';
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end - start).count();
    auto total_ops = num_threads * ops_per_thread;

    std::cout << "=== Multithreaded Benchmark ===" << std::endl;
    std::cout << "Threads:     " << num_threads << std::endl;
    std::cout << "Total ops:   " << total_ops << std::endl;
    std::cout << "Total time:  " << total_time << " ms" << std::endl;
    std::cout << "Throughput:  " << (total_ops * 1000 / total_time) << " ops/sec" << std::endl;
    std::cout << std::endl;
}

// 测试池统计
void show_pool_stats() {
    auto& pool = BufferPool::instance();

    std::cout << "=== Buffer Pool Statistics ===" << std::endl;
    std::cout << "Allocated:  " << pool.allocated() << std::endl;
    std::cout << "Available:  " << pool.available() << std::endl;
    std::cout << "In use:     " << pool.in_use() << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "Buffer Pool Performance Benchmark" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;

    show_pool_stats();
    benchmark_allocation();
    benchmark_multithreaded();
    show_pool_stats();

    return 0;
}
