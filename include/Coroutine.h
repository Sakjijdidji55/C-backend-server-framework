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
#define CBSF_USE_FIBER 1
#else
#error "Unsupported platform"
#endif

class Coroutine;
class Scheduler;

extern thread_local Coroutine* tls_current_coro;
extern thread_local Scheduler* tls_current_scheduler;

inline void yield();

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
    void suspend();
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

    template <typename F, typename... Args>
    void submit(F&& f, Args&&... args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(mtx_);
        coros_.emplace_back(std::make_shared<Coroutine>(std::move(task)));
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
};

inline void yield() {
    if (tls_current_coro)
        tls_current_coro->suspend();
}

#endif // CBSF_COROUTINE_H