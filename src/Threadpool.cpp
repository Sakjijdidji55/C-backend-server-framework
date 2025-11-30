//
// Created by HP on 2025/11/5.
//

#include <stdexcept>
#include <iostream>
#include "../include/Threadpool.h"

/**
 *  线程池构造函数，初始化指定数量的工作线程
 * @param numThreads  线程池中工作线程的数量
 */
ThreadPool::ThreadPool(size_t numThreads): running_(true) {
    // 创建指定数量的工作线程
    for(size_t i = 0; i < numThreads; ++i) {
        // 使用emplace_back直接在threads_容器中构造线程
        threads_.emplace_back([this] {
                // 线程主循环，持续运行直到线程池关闭且没有待处理任务
                while(true) {
                    // 用于存储从任务队列中获取的任务
                    std::function<void()> task;
                    {
                      // 使用互斥锁保护共享资源访问
                      std::unique_lock<std::mutex> lock(this->mtx_);
                      // 等待条件变量，直到线程池关闭或任务队列不为空
                      this->cv_.wait(lock, [this] { return !this->running_ || !this->tasks_.empty(); });
                      if(!this->running_ && this->tasks_.empty())
                          // 如果线程池关闭且任务队列为空，则退出线程
                          return;
                      task = std::move(this->tasks_.front());
                      // 从任务队列中获取一个任务
                      this->tasks_.pop();
                    }
                    try {
                        task();
                        // 执行任务
                    } catch (const std::exception&) {
                        // 捕获并处理标准异常
                        std::cerr << "Exception in thread pool thread" << std::endl;
                    } catch (...) {
                        // 捕获并处理标准异常
                        std::cerr << "Exception in thread pool thread" << std::endl;
                    }
                }
            }
        );
    }
}


/**
 * ThreadPool析构函数
 * 用于清理线程池资源
 */
ThreadPool::~ThreadPool() {
    // 使用互斥锁保护共享变量running_
    std::unique_lock<std::mutex> lock(mtx_);
    // 设置线程池运行状态为false，通知所有线程退出
    running_ = false;
    // 唤醒所有等待的线程
    cv_.notify_all();
    // 解锁互斥锁
    lock.unlock();

    // 遍历所有线程
    for(std::thread &thread : threads_)
        // 检查线程是否可被join，如果是则等待线程结束
        if(thread.joinable())
            thread.join();
}
