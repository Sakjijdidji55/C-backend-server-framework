/**
 * @file Log.cpp
 * @brief 日志类实现文件
 * @brief Log Class Implementation File
 * 
 * 此文件实现了基于单例模式的日志记录器，提供简单的日志写入功能。
 * This file implements a logger based on singleton pattern, providing simple log writing functionality.
 */
//
// Created by HP on 2025/11/19.
//

#include "../include/Log.h"

/**
 * @brief 静态成员初始化
 * @brief Static member initialization
 */
Log* Log::instance = nullptr;

/**
 * @brief 构造函数
 * @brief Constructor
 * @note 默认构造函数，使用默认初始化
 * @note Default constructor, using default initialization
 */
Log::Log() = default;

/**
 * @brief 析构函数
 * @brief Destructor
 * @note 默认析构函数
 * @note Default destructor
 */
Log::~Log() = default;

/**
 * @brief 写入日志方法
 * @param msg 要写入的日志消息
 * @brief Write log message method
 * @param msg Log message to write
 * @note 以追加模式打开日志文件，写入消息后关闭文件
 * @note Opens log file in append mode, writes message and then closes file
 */
void Log::write(const std::string& msg) {
    fp = fopen(path.c_str(), "a+");  // Open log file in append mode
    fprintf(fp, "%s\n", msg.c_str());  // Write message to log file
    fclose(fp);  // Close log file after writing
}

/**
 * @brief 获取单例实例
 * @return Log类的单例指针
 * @brief Get singleton instance
 * @return Singleton pointer of Log class
 * @note 实现单例模式，确保全局只有一个Log实例
 * @note Implements singleton pattern, ensuring only one Log instance exists globally
 * @warning 此实现不是线程安全的
 * @warning This implementation is not thread-safe
 */
Log* Log::getInstance() {
    if (instance == nullptr) {
        instance = new Log();  // Create singleton instance if not exists
    }
    return instance;
}