#ifndef CBSF_CHANNEL_H
#define CBSF_CHANNEL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>   // 用于异常处理
#include <utility>     // 用于std::move

template <typename T>
class Channel {
public:
    // 构造函数：初始化缓冲区大小（0=无缓冲通道）
    Channel(size_t buf = 0) : buffer_size(buf), closed(false) {}

    // 禁止拷贝和移动（通道是独占资源，避免多线程冲突）
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    // 推送元素（核心方法）
    void push(T val) {
        std::unique_lock<std::mutex> lock(mtx);

        // 等待条件：队列不满 或 通道关闭
        cv.wait(lock, [this] {
            if (closed) return true;
            if (buffer_size == 0) return queue.empty();
            return queue.size() < buffer_size;
        });

        // 通道已关闭，拒绝写入
        if (closed) {
            throw std::runtime_error("push to closed channel");
        }

        // 移动语义添加元素，减少拷贝
        queue.push(std::move(val));
        // 唤醒一个等待的消费者
        cv.notify_one();
    }

    // 取出元素（核心方法，抛异常）
    T pop() {
        std::unique_lock<std::mutex> lock(mtx);

        // 等待条件：队列非空 或 通道关闭
        cv.wait(lock, [this] { return !queue.empty() || closed; });

        // 通道关闭且队列为空，抛异常
        if (closed && queue.empty()) {
            throw std::runtime_error("pop from closed and empty channel");
        }

        T val = queue.front();
        queue.pop();
        // 唤醒一个等待的生产者（队列有空位）
        cv.notify_one();

        return val;
    }

    // 重载 << 运算符：简化写入（等价于push）
    Channel& operator<<(T val) { // 返回引用支持链式调用（ch << 1 << 2;）
        push(std::move(val)); // 用move优化性能
        return *this;
    }

    // 重载 >> 运算符：简化读取（等价于pop，通过引用返回值）
    // 返回bool：true=读取成功，false=通道关闭且空
    bool operator>>(T &val) {
        std::unique_lock<std::mutex> lock(mtx);

        // 等待条件：队列非空 或 通道关闭
        cv.wait(lock, [this] { return !queue.empty() || closed; });

        // 通道关闭且队列为空，读取失败
        if (closed && queue.empty()) {
            return false;
        }

        // 正常读取
        val = queue.front();
        queue.pop();
        // 唤醒生产者
        cv.notify_one();

        return true;
    }

    // 关闭通道（线程安全）
    void close() {
        std::unique_lock<std::mutex> lock(mtx);
        if (closed) {
            return; // 避免重复关闭
        }
        closed = true;
        cv.notify_all(); // 唤醒所有等待的线程（生产者+消费者）
    }

    // 判断通道是否关闭（线程安全）
    bool is_closed() const {
        std::unique_lock<std::mutex> lock(mtx);
        return closed;
    }

    // 获取当前队列大小（线程安全，可选）
    size_t size() const {
        std::unique_lock<std::mutex> lock(mtx);
        return queue.size();
    }

private:
    std::queue<T> queue;          // 元素存储队列
    mutable std::mutex mtx;       // mutable：const方法也能加锁
    std::condition_variable cv;   // 条件变量（阻塞/唤醒线程）
    size_t buffer_size;           // 缓冲区大小（0=无缓冲）
    bool closed;                  // 通道关闭标识
};

#endif //CBSF_CHANNEL_H