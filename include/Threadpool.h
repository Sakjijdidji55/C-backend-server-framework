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

class ThreadPool {
    const int maxTasks_ = 10000;
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
public:
    explicit ThreadPool(size_t numThreads) ;

    ~ThreadPool() ;

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
