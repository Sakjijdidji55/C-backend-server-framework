//
// Created by HP on 2025/11/19.
// Log class implementation file

#include "../include/Log.h"

Log* Log::instance = nullptr;

// 构造函数 - 打开日志文件
// Constructor - Open log file
Log::Log() {
    fp = fopen(path.c_str(), "a+");  // Open log file in append mode
}

// 析构函数 - 关闭日志文件
// Destructor - Close log file
Log::~Log() {
    fclose(fp);  // Close log file when destructed
}

// 写入日志方法
// Write log message method
void Log::write(const std::string& msg) {
    fprintf(fp, "%s\n", msg.c_str());  // Write message to log file
}

// 获取单例实例
// Get singleton instance
Log* Log::getInstance() {
    if (instance == nullptr) {
        instance = new Log();  // Create singleton instance if not exists
    }
    return instance;
}