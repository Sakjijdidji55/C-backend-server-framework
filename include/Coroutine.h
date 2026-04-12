#ifndef CBSF_COROUTINE_H
#define CBSF_COROUTINE_H

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <cassert>
#include <thread>
#include <atomic>
#include <stdexcept>

// 平台相关上下文切换头文件
#ifdef __linux__
#include <ucontext.h>
    #define CBSF_USE_UCONTEXT 1
#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <iostream>
#include <condition_variable>

#define CBSF_USE_FIBER 1
#else
#error "Unsupported platform"
#endif

// 前向声明
class Coroutine;
class Scheduler;

// 线程局部变量
extern thread_local Coroutine* tls_current_coro;
extern thread_local Scheduler* tls_current_scheduler;

// 主动挂起当前协程（必须在协程内部调用）
inline void yield();

// 协程状态
enum class CoroutineState : int {
    READY,
    RUNNING,
    SUSPEND,
    DONE
};

class Coroutine {
public:
    using Func = std::function<void()>;

    explicit Coroutine(Func func);
    ~Coroutine();

    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;

    Coroutine(Coroutine&& other) noexcept;
    void resume();
    void suspend();   // 由 yield 调用
    CoroutineState state() const { return state_; }

private:
#ifdef CBSF_USE_UCONTEXT
    static void entry(intptr_t arg);
#elif defined(CBSF_USE_FIBER)
    static void WINAPI fiberEntry(LPVOID lpParameter);
#endif

    Func func_;
    CoroutineState state_;

#ifdef CBSF_USE_UCONTEXT
    ucontext_t ctx_;
    std::unique_ptr<char[]> stack_;
    size_t stack_size_ = 0;
#elif defined(CBSF_USE_FIBER)
    LPVOID fiber_ = nullptr;
#endif

};

class Scheduler {
public:
    static Scheduler& instance() {
        static Scheduler ins;
        return ins;
    }

/**
 * 提交一个任务到协程调度器
 * @tparam F 可调用对象的类型
 * @tparam Args 可调用对象的参数类型
 * @param f 可调用对象（函数、函数对象、lambda表达式等）
 * @param args 可变参数，传递给可调用对象的参数
 * @note 使用完美转发避免不必要的拷贝和移动
 */
    template <typename F, typename... Args>
    void submit(F&& f, Args&&... args) {
//        std::cout<<&instance()<<std::endl;  // 输出实例信息，可能用于调试或日志
    // 使用std::bind和完美转发创建任务，保留原始函数的引用和参数
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    // 使用互斥锁保证线程安全
        std::lock_guard<std::mutex> lock(mtx_);
    // 将创建的任务封装到协程中，并添加到协程列表
        // ✅ 每次 submit 都创建一个全新的协程！
        coros_.emplace_back(std::make_shared<Coroutine>(std::move(task)));

        // ✅ 关键：提交完立刻唤醒调度器
        cv_.notify_one();
    }

    void start();

    [[noreturn]] void run_on_worker_thread();

#ifdef CBSF_USE_UCONTEXT
    ucontext_t main_ctx_;
#elif defined(CBSF_USE_FIBER)
    LPVOID main_fiber_ = nullptr;
#endif

private:
    Scheduler() = default;
    ~Scheduler() = default;

    std::vector<std::shared_ptr<Coroutine>> coros_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
};

inline void yield() {
    if (tls_current_coro)
        tls_current_coro->suspend();
}

#endif // CBSF_COROUTINE_H