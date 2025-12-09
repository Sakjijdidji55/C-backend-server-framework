/**
 * @file ThreadPool.h
 * @brief 线程池类头文件
 * @brief Thread Pool Class Header File
 */
#ifndef CBSF_THREADPOOL_H
#define CBSF_THREADPOOL_H

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @class ThreadPool
 * @brief 线程池类，用于管理多线程执行任务
 * @brief Thread Pool Class, used to manage multi-thread task execution
 * @note 提供线程安全的任务队列和线程管理功能
 * @note Provides thread-safe task queue and thread management functionality
 */
class ThreadPool {
private:
    /**
     * @brief 最大任务数
     * @brief Maximum number of tasks
     */
    const int maxTasks_ = 10000;
    
    /**
     * @brief 线程列表
     * @brief Thread list
     */
    std::vector<std::thread> threads_;
    
    /**
     * @brief 任务队列
     * @brief Task queue
     */
    std::queue<std::function<void()>> tasks_;
    
    /**
     * @brief 互斥锁，保护任务队列
     * @brief Mutex lock, protects the task queue
     */
    std::mutex mtx_;
    
    /**
     * @brief 条件变量，用于线程同步
     * @brief Condition variable, used for thread synchronization
     */
    std::condition_variable cv_;
    
    /**
     * @brief 运行标志，表示线程池是否正在运行
     * @brief Running flag, indicates whether the thread pool is running
     */
    std::atomic<bool> running_;
    
public:
    /**
     * @brief 构造函数，初始化线程池
     * @param numThreads 线程池中的线程数量
     * @brief Constructor, initialize thread pool
     * @param numThreads Number of threads in the thread pool
     */
    explicit ThreadPool(size_t numThreads);

    /**
     * @brief 析构函数，释放线程池资源
     * @brief Destructor, release thread pool resources
     */
    ~ThreadPool();

    /**
     * @brief 添加任务到任务队列中
     * @tparam F 函数类型
     * @tparam Args 可变参数类型
     * @param f 要添加的函数
     * @param args 函数参数
     * @return 添加成功返回true，队列已满返回false
     * @brief Add task to task queue
     * @tparam F Function type
     * @tparam Args Variable parameter types
     * @param f Function to add
     * @param args Function arguments
     * @return Returns true if added successfully, false if queue is full
     */
    template<typename F, typename... Args>
    bool addTask(F&& f, Args&&... args) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 检查线程池是否正在运行
        // Check if thread pool is running
        if (!running_) {
            return false;
        }
        // 检查任务队列是否已满
        // Check if task queue is full
        if (tasks_.size() >= static_cast<size_t>(maxTasks_)) {
            return false;
        }
        // 将任务添加到队列中
        // Add task to the queue
        tasks_.emplace(std::bind(
                std::forward<F>(f),
                std::forward<Args>(args)...
        ));
        lock.unlock();
        // 通知一个等待的线程有新任务可用
        // Notify one waiting thread that a new task is available
        cv_.notify_one();
        return true;
    }
};


#endif //CBSF_THREADPOOL_H
