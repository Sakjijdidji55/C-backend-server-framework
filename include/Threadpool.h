//
// Created by HP on 2025/11/5.
//

#ifndef FLIGHTSERVER_THREADPOOL_H
#define FLIGHTSERVER_THREADPOOL_H

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

// 线程池类，用于管理多线程执行任务
// Thread pool class, used to manage multi-thread task execution
class ThreadPool {
private:
    // 最大任务数
    // Maximum number of tasks
    const int maxTasks_ = 10000;
    // 线程列表
    // Thread list
    std::vector<std::thread> threads_;
    // 任务队列
    // Task queue
    std::queue<std::function<void()>> tasks_;
    // 互斥锁
    // Mutex lock
    std::mutex mtx_;
    // 条件变量
    // Condition variable
    std::condition_variable cv_;
    // 运行标志
    // Running flag
    std::atomic<bool> running_;
public:
    // 构造函数，初始化线程池
    // Constructor, initialize thread pool
    explicit ThreadPool(size_t numThreads) ;

    // 析构函数，释放线程池资源
    // Destructor, release thread pool resources
    ~ThreadPool() ;

    // 添加任务到线程池
    // Add task to thread pool
    /**
     * @brief 添加任务到任务队列中
     * @tparam F 函数类型
     * @tparam Args 可变参数类型
     * @param f 要添加的函数
     * @param args 函数参数
     * @return 添加成功返回true，队列已满返回false
     */
    template<typename F, typename... Args>
    bool addTask(F&& f, Args&&... args) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!running_) {
            return false;
        }
        if (tasks_.size() >= static_cast<size_t>(maxTasks_)) {
            return false;
        }
        tasks_.emplace(std::bind(
                std::forward<F>(f),
                std::forward<Args>(args)...
        ));
        lock.unlock();
        cv_.notify_one();
        return true;
    }
};


#endif //FLIGHTSERVER_THREADPOOL_H
