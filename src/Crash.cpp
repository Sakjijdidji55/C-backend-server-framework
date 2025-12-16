//
// Created by HP on 2025/12/16.
//
#include "Crash.h"
#include <sstream>
#include <fstream>
#include <iomanip>

// 平台特有头文件
#include <iostream>
#include <string>

// 平台特有头文件：严格隔离 Windows/Linux
#ifdef _WIN32  // 仅 Windows 平台包含 Windows 专属头文件
#include <windows.h>
#include <tlhelp32.h> // MODULEENTRY32 依赖
#include <dbghelp.h>  // Windows 调试帮助库（仅 Windows 可见）
#else  // Linux/Mac 平台：替换为系统原生调试接口
#include <execinfo.h>  // backtrace/backtrace_symbols
#include <signal.h>    // 信号处理（捕获崩溃）
#include <unistd.h>    // POSIX 系统调用
#include <stdlib.h>    // exit/free
#endif

// 初始化崩溃处理
void CrashHandler::init(const CleanupCallback& cleanupFunc) {
    cleanupCallback_ = cleanupFunc;
    isExiting_ = false;

    // 1. 注册未捕获C++异常处理
    std::set_terminate(terminateHandler);

    // 2. 注册信号处理（覆盖所有致命信号）
    const int signals[] = {
            SIGINT,    // Ctrl+C
            SIGTERM,   // 终止信号（kill）
            SIGSEGV,   // 段错误（空指针、越界）
            SIGABRT,   // abort()调用
            SIGILL,    // 非法指令
            SIGFPE,    // 浮点错误
#ifdef __linux__
            SIGBUS     // 总线错误（仅Linux支持，Windows无此信号）
#endif
    };

    for (int sig : signals) {
#ifdef _WIN32
        // Windows signal API
        if (signal(sig, signalHandler) == SIG_ERR) {
            std::cerr << "Failed to register signal handler for sig: " << sig << std::endl;
        }
#else
        // Linux sigaction API（更可靠）
        struct sigaction sa;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(sig, &sa, nullptr) == -1) {
            std::cerr << "Failed to register sigaction for sig: " << sig << std::endl;
        }
#endif
    }

    // 3. 平台特有处理
#ifdef _WIN32
    // Windows结构化异常（SEH）
    SetUnhandledExceptionFilter(sehExceptionHandler);
    // Windows控制台关闭事件
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else
    // Linux额外注册SIGQUIT（Ctrl+\）
    struct sigaction sa_quit;
    sa_quit.sa_handler = signalHandler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = SA_RESTART;
    sigaction(SIGQUIT, &sa_quit, nullptr);
#endif

    std::cout << "Crash handler initialized successfully" << std::endl;
}

// 通用优雅退出逻辑
void CrashHandler::gracefulExit(int exitCode, const std::string& reason) {
    // 防止多线程重复触发退出
    if (isExiting_.exchange(true)) {
        return;
    }

    std::lock_guard<std::mutex> lock(exitMutex_);

    // 1. 打印崩溃/退出原因
    std::cerr << "\n=====================================" << std::endl;
    std::cerr << "          CRASH/EXIT DETECTED         " << std::endl;
    std::cerr << "=====================================" << std::endl;
    std::cerr << "Reason: " << reason << std::endl;
    std::cerr << "Exit code: " << exitCode << std::endl;

    // 2. 打印崩溃信息（回溯）
    printCrashInfo(exitCode, reason);

    // 3. 执行用户自定义清理逻辑
    if (cleanupCallback_) {
        try {
            std::cerr << "\nExecuting cleanup logic..." << std::endl;
            cleanupCallback_(); // 执行服务器/业务层清理
        } catch (const std::exception& e) {
            std::cerr << "Cleanup failed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Cleanup failed: Unknown exception" << std::endl;
        }
    }

    // 4. 最终退出（确保资源释放）
    std::cerr << "\nGraceful exit completed. Exiting..." << std::endl;
#ifdef _WIN32
    // Windows安全退出API
    ExitProcess(static_cast<UINT>(exitCode));
#else
    // Linux退出API（避免析构函数重复执行）
    _exit(exitCode);
#endif
}

// 信号处理函数
void CrashHandler::signalHandler(int sig) {
    std::string reason;
    switch (sig) {
        case SIGINT: reason = "SIGINT (Ctrl+C)"; break;
        case SIGTERM: reason = "SIGTERM (Kill command)"; break;
        case SIGSEGV: reason = "SIGSEGV (Segmentation fault)"; break;
        case SIGABRT: reason = "SIGABRT (Abort called)"; break;
        case SIGILL: reason = "SIGILL (Illegal instruction)"; break;
        case SIGFPE: reason = "SIGFPE (Floating point error)"; break;
#ifdef __linux__
            case SIGBUS: reason = "SIGBUS (Bus error)"; break;
        case SIGQUIT: reason = "SIGQUIT (Ctrl+\\)"; break;
#endif
        default: reason = "Unknown signal (" + std::to_string(sig) + ")";
    }
    instance().gracefulExit(sig, reason);
}
// Windows SEH异常处理（捕获内存错误、空指针等）
#ifdef _WIN32
LONG WINAPI CrashHandler::sehExceptionHandler(EXCEPTION_POINTERS* pException) {
    std::string reason;
    DWORD exceptionCode = pException ? pException->ExceptionRecord->ExceptionCode : 0;

    switch (exceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            reason = "EXCEPTION_ACCESS_VIOLATION (Invalid memory access)"; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            reason = "EXCEPTION_INT_DIVIDE_BY_ZERO (Integer divide by zero)"; break;
        case EXCEPTION_STACK_OVERFLOW:
            reason = "EXCEPTION_STACK_OVERFLOW (Stack overflow)"; break;
        default: {
            std::ostringstream oss;
            oss << "SEH Exception (0x" << std::hex << exceptionCode << std::dec << ")";
            reason = oss.str();
            break;
        }
    }

    // MinGW 下直接使用传入的 pException，不调用 GetExceptionInformation
    instance().gracefulExit(static_cast<int>(exceptionCode), reason);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// Windows控制台关闭事件
#ifdef _WIN32
BOOL WINAPI CrashHandler::consoleCtrlHandler(DWORD dwCtrlType) {
    std::string reason;
    switch (dwCtrlType) {
        case CTRL_C_EVENT: reason = "CTRL_C_EVENT (Ctrl+C)"; break;
        case CTRL_CLOSE_EVENT: reason = "CTRL_CLOSE_EVENT (Console window closed)"; break;
        case CTRL_BREAK_EVENT: reason = "CTRL_BREAK_EVENT (Ctrl+Break)"; break;
        case CTRL_SHUTDOWN_EVENT: reason = "CTRL_SHUTDOWN_EVENT (System shutdown)"; break;
        case CTRL_LOGOFF_EVENT: reason = "CTRL_LOGOFF_EVENT (User logoff)"; break;
        default: {
            std::ostringstream oss;
            oss << "Unknown console event (0x" << std::hex << dwCtrlType << std::dec << ")";
            reason = oss.str();
            break;
        }
    }
    instance().gracefulExit(static_cast<int>(dwCtrlType), reason);
    return TRUE;
}
#endif

// 未捕获C++异常处理
void CrashHandler::terminateHandler() {
    std::string reason = "Uncaught C++ exception";
    try {
        throw; // 重新抛出异常，获取具体信息
    } catch (const std::exception& e) {
        reason += ": " + std::string(e.what());
    } catch (...) {
        reason += ": Unknown exception type";
    }
    instance().gracefulExit(EXIT_FAILURE, reason);
}

// 辅助函数：格式化时间（跨平台）
static std::string formatCurrentTime() {
    std::time_t now = std::time(nullptr);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &now); // Windows安全版localtime
#else
    localtime_r(&now, &tm_buf); // Linux安全版localtime
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 打印崩溃信息（分编译器/平台适配）
// 修复 printCrashInfo 函数中的条件编译配对（核心）
void CrashHandler::printCrashInfo(int sig, const std::string& reason) {
    (void)sig; // 避免未使用警告

#if defined(COMPILER_MSVC)
    // ========== MSVC 编译器（完整栈回溯） ==========
    std::cerr << "\nCrash info (Windows/MSVC):" << std::endl;
    std::cerr << "  Time: " << formatCurrentTime() << std::endl;

    // MSVC 专属逻辑（省略，保持原有）
#elif defined(COMPILER_MINGW)
    // ========== MinGW 编译器（简化版） ==========
    std::cerr << "\nCrash info (Windows/MinGW):" << std::endl;
    std::cerr << "  Time: " << formatCurrentTime() << std::endl;
    std::cerr << "  Reason: " << reason << std::endl;

    // MinGW 下打印已加载模块（需包含 TlHelp32.h）
    HANDLE hProcess = GetCurrentProcess();
    if (SymInitialize(hProcess, nullptr, TRUE)) {
        std::cerr << "  Loaded modules (base addresses):" << std::endl;

        MODULEENTRY32 me32{};
        me32.dwSize = sizeof(MODULEENTRY32);
        // 修正：MinGW 下 TH32CS_SNAPMODULE32 可能未定义，兼容处理
        DWORD dwFlags = TH32CS_SNAPMODULE;
#ifdef TH32CS_SNAPMODULE32
        dwFlags |= TH32CS_SNAPMODULE32;
#endif
        HANDLE hSnapshot = CreateToolhelp32Snapshot(dwFlags, GetCurrentProcessId());

        if (hSnapshot != INVALID_HANDLE_VALUE) {
            if (Module32First(hSnapshot, &me32)) {
                int modCount = 0;
                do {
                    if (modCount < 10) { // 仅打印前10个核心模块
                        std::cerr << "    - " << me32.szModule
                                  << " (0x" << std::hex << (UINT64)me32.modBaseAddr << std::dec << ")" << std::endl;
                    }
                    modCount++;
                } while (Module32Next(hSnapshot, &me32));
            }
            CloseHandle(hSnapshot);
        }
        SymCleanup(hProcess);
    }
    std::cerr << "  Note: MinGW does not support SEH stack backtrace (use MSVC for full debug)" << std::endl;
#elif defined(COMPILER_LINUX)
    // ========== Linux 编译器 ==========
    std::cerr << "\nCrash info (Linux):" << std::endl;
    std::cerr << "  Time: " << formatCurrentTime() << std::endl;
    // Linux 专属逻辑（省略，保持原有）
#endif // 关键：添加匹配的 #endif，修复配对错误

    // 跨平台日志写入（保持原有）
    try {
        std::ofstream logFile("crash_log.txt", std::ios::app);
        if (logFile.is_open()) {
            logFile << "[" << formatCurrentTime() << "] "
                    << "ExitCode: " << sig << ", Reason: " << reason << "\n";
#if defined(COMPILER_MSVC)
            logFile << "  Compiler: MSVC\n";
#elif defined(COMPILER_MINGW)
            logFile << "  Compiler: MinGW\n";
#elif defined(COMPILER_LINUX)
            logFile << "  Compiler: " << (__clang__ ? "Clang" : "GCC") << "\n";
#endif
            logFile << "----------------------------------------\n";
            logFile.close();
        }
    } catch (...) {}
}