// @file RDConnector.h
// @brief Redis数据库连接器类头文件
// 提供Redis数据库连接和基本操作接口，实现了单例模式
// Redis database connector class header file
// Provides Redis database connection and basic operation interfaces, implements singleton pattern

#ifndef CBSF_RECONNECTOR_H
#define CBSF_RECONNECTOR_H

#include <string>
#include <iostream>
#include <hiredis/hiredis.h>

/**
 * @class RdConnector
 * @brief Redis数据库连接器类
 * Redis database connector class
 * 提供与Redis服务器的连接管理和基本数据操作功能
 * Provides connection management and basic data operation functions with Redis server
 */
class RdConnector {
    std::string host_;         ///< Redis服务器主机地址 Redis server host address
    std::string port_;         ///< Redis服务器端口号 Redis server port number
    std::string password_;     ///< Redis服务器密码 Redis server password
    int db;                    ///< Redis数据库索引 Redis database index
    redisContext* context;     ///< Redis连接上下文 Redis connection context

    static RdConnector* instance_;  ///< 单例实例 Singleton instance
public:
    /**
     * @brief 构造函数
     * Constructor
     * @param host Redis服务器主机地址 Redis server host address
     * @param port Redis服务器端口号 Redis server port number
     * @param password Redis服务器密码 Redis server password
     * @param db Redis数据库索引 Redis database index
     */
    RdConnector(std::string host, std::string port, std::string password, int db) : host_(std::move(host)), port_(std::move(port)), password_(std::move(password)), db(db), context(
            nullptr) {
    }

    /**
     * @brief 析构函数
     * Destructor
     * 释放Redis连接资源
     * Releases Redis connection resources
     */
    ~RdConnector() {
        if(context != nullptr){
            redisFree(context);  // 释放Redis连接 Free Redis connection
            context = nullptr;
            std::cout<<"redis context free"<<std::endl;
        }
    }

    /**
     * @brief 连接到Redis服务器
     * Connect to Redis server
     * @return 连接成功返回true，失败返回false
     * Returns true if connection succeeds, false otherwise
     */
    bool connect() ;

    /**
     * @brief 获取指定键的值
     * Get the value of the specified key
     * @param key 要获取的键名 Key name to get
     * @return 键对应的值，若键不存在或发生错误则返回空字符串
     * The value corresponding to the key, empty string if key does not exist or error occurs
     */
    std::string get(const std::string& key) ;

    /**
     * @brief 设置键值对
     * Set key-value pair
     * @param key 键名 Key name
     * @param value 值 Value
     * @return 设置成功返回true，失败返回false
     * Returns true if set succeeds, false otherwise
     */
    bool set(const std::string& key, const std::string& value) ;

    /**
     * @brief 设置带过期时间的键值对
     * Set key-value pair with expiration time
     * @param key 键名 Key name
     * @param value 值 Value
     * @param expireSeconds 过期时间（秒） Expiration time in seconds
     * @return 设置成功返回true，失败返回false
     * Returns true if set succeeds, false otherwise
     */
    bool set(const std::string& key, const std::string& value, int expireSeconds) ;

    /**
     * @brief 检查键是否存在
     * Check if key exists
     * @param key 要检查的键名 Key name to check
     * @return 键存在返回true，不存在返回false
     * Returns true if key exists, false otherwise
     */
    bool exists(const std::string& key) ;

    /**
     * @brief 删除指定的键
     * Delete the specified key
     * @param key 要删除的键名 Key name to delete
     * @return 删除成功返回true，失败返回false
     * Returns true if deletion succeeds, false otherwise
     */
    bool del(const std::string& key) ;

    /**
     * @brief 获取单例实例
     * Get singleton instance
     * @return RdConnector类的单例实例
     * The singleton instance of RdConnector class
     */
    static RdConnector* getInstance();

    /**
     * @brief 获取连接错误信息
     * Get connection error message
     * @return 错误信息字符串
     * Error message string
     */
    std::string getError();
};

#endif //CBSF_RECONNECTOR_H
