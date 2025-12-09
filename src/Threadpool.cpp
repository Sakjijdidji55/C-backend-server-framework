/**
 * @file Threadpool.cpp
 * @brief 线程池实现文件
 * @brief Thread Pool Implementation File
 * 
 * 此文件实现了一个简单的线程池，支持动态任务提交和线程管理，
 * 采用生产者-消费者模式处理并发任务。
 * This file implements a simple thread pool that supports dynamic task submission
 * and thread management, using producer-consumer pattern for concurrent task processing.
 */
//
// Created by HP on 2025/11/5.
//

#include <stdexcept>
#include <iostream>
#include "../include/Threadpool.h"

/**
 * @brief 线程池构造函数，初始化指定数量的工作线程
 * @param numThreads 线程池中工作线程的数量
 * @brief Thread pool constructor, initializes specified number of worker threads
 * @param numThreads Number of worker threads in the thread pool
 * @note 构造函数创建并启动工作线程，每个线程会等待任务
 * @note Constructor creates and starts worker threads, each waiting for tasks
 */
ThreadPool::ThreadPool(size_t numThreads): running_(true) {
    // 创建指定数量的工作线程
    // Create specified number of worker threads
    for(size_t i = 0; i < numThreads; ++i) {
        // 使用emplace_back直接在threads_容器中构造线程
        threads_.emplace_back([this] {
                // 线程主循环，持续运行直到线程池关闭且没有待处理任务
                while(true) {
                    // 用于存储从任务队列中获取的任务
                    std::function<void()> task;
                    {
                      // 使用互斥锁保护共享资源访问
                    // Use mutex to protect shared resource access
                    std::unique_lock<std::mutex> lock(this->mtx_);
                    // 等待条件变量，直到线程池关闭或任务队列不为空
                    // Wait on condition variable until thread pool is closed or task queue is not empty
                    this->cv_.wait(lock, [this] { return !this->running_ || !this->tasks_.empty(); });
                    if(!this->running_ && this->tasks_.empty())
                        // 如果线程池关闭且任务队列为空，则退出线程
                        // Exit thread if pool is closed and task queue is empty
                        return;
                    task = std::move(this->tasks_.front());
                    // 从任务队列中获取一个任务
                    // Get a task from the task queue
                    this->tasks_.pop();
                    }
                    try {
                        task();
                        // 执行任务
                        // Execute task
                    } catch (const std::exception&) {
                        // 捕获并处理标准异常
                        // Catch and handle standard exceptions
                        std::cerr << "Exception in thread pool thread" << std::endl;
                    } catch (...) {
                        // 捕获并处理所有其他异常
                        // Catch and handle all other exceptions
                        std::cerr << "Exception in thread pool thread" << std::endl;
                    }
                }
            }
        );
    }
}


/**
 * @brief 线程池析构函数
 * @brief ThreadPool destructor
 * @note 停止线程池并等待所有工作线程结束
 * @note Stops the thread pool and waits for all worker threads to finish
 */
ThreadPool::~ThreadPool() {
    // 使用互斥锁保护共享变量running_
    // Use mutex to protect shared variable running_
    std::unique_lock<std::mutex> lock(mtx_);
    // 设置线程池运行状态为false，通知所有线程退出
    // Set thread pool running state to false, notify all threads to exit
    running_ = false;
    // 唤醒所有等待的线程
    // Wake up all waiting threads
    cv_.notify_all();
    // 解锁互斥锁
    // Unlock mutex
    lock.unlock();

    // 遍历所有线程
    // Iterate over all threads
    for(std::thread &thread : threads_)
        // 检查线程是否可被join，如果是则等待线程结束
        // Check if thread is joinable, wait for thread to finish if it is
        if(thread.joinable())
            thread.join();
}
