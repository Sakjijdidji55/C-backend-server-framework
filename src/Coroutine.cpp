//
// Created by HP on 2026/4/9.
//

#include "Coroutine.h"

// ==================== Coroutine 实现 ====================
constexpr size_t kDefaultStackSize = 128 * 1024;
thread_local Coroutine* tls_current_coro;
thread_local Scheduler* tls_current_scheduler = nullptr;

#ifdef CBSF_USE_UCONTEXT
void Coroutine::entry(intptr_t arg) {
    Coroutine* coro = reinterpret_cast<Coroutine*>(arg);
    if (coro->func_) coro->func_();
    coro->state_ = CoroutineState::DONE;
    yield();   // 返回调度器
}
#elif defined(CBSF_USE_FIBER)
void WINAPI Coroutine::fiberEntry(LPVOID lpParameter) {
    Coroutine* coro = static_cast<Coroutine*>(lpParameter);
    if (coro->func_) coro->func_();
    coro->state_ = CoroutineState::DONE;
    yield();
}
#endif

Coroutine::Coroutine(Func func)
        : func_(std::move(func)), state_(CoroutineState::READY) {
#ifdef CBSF_USE_UCONTEXT
    stack_size_ = kDefaultStackSize;
    stack_.reset(new char[stack_size_]);
    if (getcontext(&ctx_) == -1)
        throw std::runtime_error("getcontext failed");
    ctx_.uc_stack.ss_sp = stack_.get();
    ctx_.uc_stack.ss_size = stack_size_;
    ctx_.uc_link = nullptr;
    makecontext(&ctx_, (void(*)())entry, 1, reinterpret_cast<intptr_t>(this));
#elif defined(CBSF_USE_FIBER)
    fiber_ = CreateFiber(kDefaultStackSize, fiberEntry, this);
    if (!fiber_)
        throw std::runtime_error("CreateFiber failed");
#endif
}

Coroutine::~Coroutine() {
#ifdef CBSF_USE_FIBER
    if (fiber_) DeleteFiber(fiber_);
#endif
}

Coroutine::Coroutine(Coroutine&& other) noexcept
        : func_(std::move(other.func_)),
          state_(other.state_)
#ifdef CBSF_USE_UCONTEXT
        , ctx_(other.ctx_),
      stack_(std::move(other.stack_)),
      stack_size_(other.stack_size_)
#elif defined(CBSF_USE_FIBER)
        , fiber_(other.fiber_)
#endif
{
    other.state_ = CoroutineState::DONE;
#ifdef CBSF_USE_FIBER
    other.fiber_ = nullptr;
#endif
}

// resume 和 suspend 需要访问 Scheduler 的成员，所以定义在 Scheduler 之后
void Coroutine::resume() {
    if (state_ == CoroutineState::DONE) return;
    Scheduler* sched = tls_current_scheduler;
    if (!sched) return;

    state_ = CoroutineState::RUNNING;
    tls_current_coro = this;

#ifdef CBSF_USE_UCONTEXT
    swapcontext(&sched->main_ctx_, &ctx_);
#elif defined(CBSF_USE_FIBER)
    SwitchToFiber(fiber_);
#endif

    if (state_ == CoroutineState::DONE) {
        // 协程已结束，资源稍后释放
    }
}

void Coroutine::suspend() {
    if (state_ != CoroutineState::RUNNING) return;
    state_ = CoroutineState::SUSPEND;

    Scheduler* sched = tls_current_scheduler;
    if (!sched) return;

#ifdef CBSF_USE_UCONTEXT
    swapcontext(&ctx_, &sched->main_ctx_);
#elif defined(CBSF_USE_FIBER)
    SwitchToFiber(sched->main_fiber_);
#endif
}

// ==================== Scheduler 实现 ====================
void Scheduler::start() {
    tls_current_scheduler = this;

#ifdef CBSF_USE_FIBER
    main_fiber_ = ConvertThreadToFiber(nullptr);
    if (!main_fiber_)
        throw std::runtime_error("ConvertThreadToFiber failed");
#endif

    while (true) {
        std::vector<std::shared_ptr<Coroutine>> run_list;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (coros_.empty()) break;
            run_list.swap(coros_);
        }
        for (auto& coro : run_list) {
            if (coro->state() == CoroutineState::READY ||
                coro->state() == CoroutineState::SUSPEND) {
                coro->resume();
                if (coro->state() != CoroutineState::DONE) {
                    std::lock_guard<std::mutex> lock(mtx_);
                    coros_.push_back(coro);
                }
            }
        }
    }

#ifdef CBSF_USE_FIBER
    ConvertFiberToThread();
#endif
    tls_current_scheduler = nullptr;
}

void Scheduler::run_on_worker_thread() {
    tls_current_scheduler = this;

#ifdef CBSF_USE_FIBER
    main_fiber_ = ConvertThreadToFiber(nullptr);
    if (!main_fiber_)
        throw std::runtime_error("ConvertThreadToFiber failed");
#endif

    while (true) {
        std::vector<std::shared_ptr<Coroutine>> run_list;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (!coros_.empty())
                run_list.swap(coros_);
        }
        if (run_list.empty()) {
            std::this_thread::yield();
            continue;
        }
        for (auto& coro : run_list) {
            if (coro->state() == CoroutineState::READY ||
                coro->state() == CoroutineState::SUSPEND) {
                coro->resume();
                if (coro->state() != CoroutineState::DONE) {
                    std::lock_guard<std::mutex> lock(mtx_);
                    coros_.push_back(coro);
                }
            }
        }
    }

#ifdef CBSF_USE_FIBER
    ConvertFiberToThread();
#endif
    tls_current_scheduler = nullptr;
}

