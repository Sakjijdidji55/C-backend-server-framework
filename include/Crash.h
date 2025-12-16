//
// Created by HP on 2025/12/16.
//

#ifndef CBSF_CRASH_H
#define CBSF_CRASH_H

// 编译器判断宏封装
#ifdef _MSC_VER
#define COMPILER_MSVC 1
#elif __MINGW32__
#define COMPILER_MINGW 1
#elif __linux__
#define COMPILER_LINUX 1
#endif

#include <atomic>
#include <mutex>
#include <string>
#include <functional>
#include <iostream>
#include <chrono>
#include <thread>

// 跨平台兼容宏
#ifdef _WIN32
#include <windows.h>
#include <eh.h>          // SEH异常
#include <signal.h>      // Windows信号
//#pragma comment(lib, "dbghelp.lib") // 崩溃回溯（可选）
#else
#include <signal.h>
#include <execinfo.h>    // Linux崩溃回溯
#include <unistd.h>
#include <stdlib.h>
#endif

// 优雅退出回调类型（用户自定义清理逻辑）
using CleanupCallback = std::function<void()>;

class CrashHandler {
public:
    // 单例模式
    static CrashHandler& instance() {
        static CrashHandler handler;
        return handler;
    }

    // 初始化崩溃处理（注册所有捕获机制）
    void init(const CleanupCallback& cleanupFunc);

    // 触发优雅退出（主动调用/被崩溃处理触发）
    void gracefulExit(int exitCode = 1, const std::string& reason = "Unknown crash");

    // 禁止拷贝
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

private:
    CrashHandler() : cleanupCallback_(nullptr), isExiting_(false) {}
    ~CrashHandler() = default;

    // 信号处理函数（Linux/Windows通用）
    static void signalHandler(int sig);

#ifdef _WIN32
    // Windows结构化异常处理（捕获内存错误、空指针等）
    static LONG WINAPI sehExceptionHandler(EXCEPTION_POINTERS* pException);
    // Windows控制台关闭事件（Ctrl+C/关闭窗口）
    static BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType);
#endif

    // 未捕获C++异常处理
    static void terminateHandler();

    // 打印崩溃信息（可选：崩溃回溯）
    static void printCrashInfo(int sig, const std::string& reason);

    // 清理回调
    CleanupCallback cleanupCallback_;
    // 防止重复退出
    std::atomic<bool> isExiting_;
    // 退出锁
    std::mutex exitMutex_;
};

// 自定义断言宏（替换默认assert，触发优雅退出）
#define SAFE_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            CrashHandler::instance().gracefulExit(1, "Assert failed: " #expr); \
        } \
    } while (0)

#endif //CBSF_CRASH_H
