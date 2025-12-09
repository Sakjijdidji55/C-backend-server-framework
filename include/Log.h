/**
 * @file Log.h
 * @brief 日志类头文件
 * @brief Log Class Header File
 * @author HP
 * @date 2025/11/19
 */

#ifndef CBSF_LOG_H
#define CBSF_LOG_H

#include <string>

/**
 * @class Log
 * @brief 日志类，采用单例模式设计
 * @brief Log class, designed with singleton pattern
 * @note 用于记录程序运行日志
 * @note Used for recording program running logs
 */
class Log {
    std::string path = "../log.log";  ///< 日志文件路径 (Log file path)
    FILE *fp{};  ///< 文件指针 (File pointer)
    static Log *instance;  ///< 单例实例指针 (Singleton instance pointer)
    
    /**
     * @brief 构造函数 - 私有，防止外部创建实例
     * @brief Constructor - Private, prevent external instance creation
     */
    Log();
    
public:
    /**
     * @brief 析构函数
     * @brief Destructor
     * @details 关闭日志文件并释放资源
     * @details Closes log file and releases resources
     */
    ~Log();

    /**
     * @brief 写入日志方法
     * @brief Write log message method
     * @param msg 要写入的日志消息
     * @param msg Log message to write
     */
    void write(const std::string &msg);

    /**
     * @brief 获取单例实例的静态方法
     * @brief Static method to get singleton instance
     * @return 返回Log类的单例实例指针
     * @return Returns pointer to singleton instance of Log class
     */
    static Log *getInstance();
};

#endif //CBSF_LOG_H
