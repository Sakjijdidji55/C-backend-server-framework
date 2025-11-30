//
// Created by HP on 2025/11/19.
// Log class header file

#ifndef FLIGHTSERVER_LOG_H
#define FLIGHTSERVER_LOG_H

#include <string>

// 日志类，采用单例模式设计
// Log class, designed with singleton pattern
class Log {
    std::string path = "../log.log";  // 日志文件路径 (Log file path)
    FILE *fp;  // 文件指针 (File pointer)
    static Log *instance;  // 单例实例指针 (Singleton instance pointer)
    // 构造函数 - 私有，防止外部创建实例
    // Constructor - Private, prevent external instance creation
    Log();
public:
    // 析构函数
    // Destructor
    ~Log();

    // 写入日志方法
    // Write log message method
    void write(const std::string &msg);

    // 获取单例实例的静态方法
    // Static method to get singleton instance
    static Log *getInstance();
};

#endif //FLIGHTSERVER_LOG_H
