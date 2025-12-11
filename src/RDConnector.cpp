// @file RDConnector.cpp
// @brief Redis数据库连接器类实现文件
// 实现Redis连接管理和基本数据操作功能
// Redis database connector class implementation file
// Implements Redis connection management and basic data operation functions

#include "../include/RDConnector.h"

// 初始化静态单例实例指针
// Initialize static singleton instance pointer
RdConnector* RdConnector::instance_ = nullptr;

/**
 * @brief 连接到Redis服务器
 * Connect to Redis server
 * @return 连接成功返回true，失败返回false
 * Returns true if connection succeeds, false otherwise
 *
 * 该函数执行以下操作：
 * 1. 检查是否已经存在其他RdConnector实例
 * 2. 如果已存在连接，则先断开
 * 3. 将端口号字符串转换为整数
 * 4. 尝试连接到Redis服务器
 * 5. 如果配置了密码，进行身份验证
 * 6. 如果指定了非0数据库，切换到指定数据库
 * 7. 如果是第一个实例，则设置instance_指针
 *
 * This function performs the following operations:
 * 1. Check if another RdConnector instance already exists
 * 2. Disconnect if a connection already exists
 * 3. Convert port string to integer
 * 4. Try to connect to Redis server
 * 5. Authenticate if password is configured
 * 6. Switch to specified database if non-zero database is specified
 * 7. Set instance_ pointer if it's the first instance
 */
bool RdConnector::connect() {
    // 检查是否已经存在其他RdConnector实例
    // Check if another RdConnector instance already exists
    if (instance_ != nullptr && instance_ != this) {
        std::cerr << "Only one instance of RdConnector is allowed." << std::endl;
        return false;
    }

    // 如果已存在连接，则先断开
    // Disconnect if a connection already exists
    if (context != nullptr) {
        redisFree(context);
        context = nullptr;
    }
    // 将端口号字符串转换为整数
    // Convert port string to integer
    int port = -1;
    try {
        port = std::stoi(port_);
    } catch (...) {
        std::cerr << "端口格式错误：" << port << std::endl;
        return false;
    }
    // 尝试连接到Redis服务器
    // Try to connect to Redis server
    context = redisConnect(host_.c_str(), port);
    if (context == nullptr || context->err) {
        std::cerr << "连接失败：" << (context ? context->errstr : "内存分配失败") << std::endl;
        return false;
    }
    // 如果配置了密码，进行身份验证
    // Authenticate if password is configured
    if (!password_.empty()) {
        auto* reply = (redisReply*)redisCommand(context, "AUTH %s", password_.c_str());
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "密码验证失败：" << (reply ? reply->str : context->errstr) << std::endl;
            redisFree(context);
            context = nullptr;
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);  // 释放命令结果
    }
    // 如果指定了非0数据库，切换到指定数据库
    // Switch to specified database if non-zero database is specified
    if(db!=0){
        auto* reply = (redisReply*)redisCommand(context, "SELECT %d", db);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "切换数据库失败：" << (reply ? reply->str : context->errstr) << std::endl;
            redisFree(context);
            context = nullptr;
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);  // 释放命令结果
    }

    // 如果是第一个实例，则设置instance_指针
    // Set instance_ pointer if it's the first instance
    if (instance_ == nullptr) {
        instance_ = this;
    }

    return true;
}

/**
 * @brief 从Redis中获取指定键的值
 * Get the value of the specified key from Redis
 * @param key 要获取的键名 Key name to get
 * @return 返回键对应的值，如果键不存在或发生错误则返回空字符串
 * Returns the value corresponding to the key, empty string if key does not exist or error occurs
 */
std::string RdConnector::get(const std::string& key) {
    // 检查Redis连接上下文是否有效
    // Check if Redis connection context is valid
    if (context == nullptr){
        std::cerr<<"未连接"<<std::endl;
        return "";
    }
    // 执行Redis GET命令获取键值
    // Execute Redis GET command to get key value
    auto* reply = (redisReply*)redisCommand(context, "GET %s", key.c_str());
    // 检查命令执行是否成功
    // Check if command execution succeeded
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "获取失败：" << (reply ? reply->str : context->errstr) << std::endl;
        freeReplyObject(reply);
        return "";
    }
    std::string result;
    // 根据返回类型处理结果
    // Process result according to return type
    if (reply->type == REDIS_REPLY_STRING) {
        result = reply->str;  // 键存在，返回值
    } else if (reply->type == REDIS_REPLY_NIL) {
        result = "";  // 键不存在，返回空
    } else {
        // 处理其他异常情况
        std::cerr << "GET 命令返回异常（类型：" << reply->type << "）" << std::endl;
        result = "";
    }
    // 释放Redis回复对象
    // Free Redis reply object
    freeReplyObject(reply);
    return result;
}

/**
 * @brief 设置Redis键值对
 * Set Redis key-value pair
 * @param key 要设置的键 Key to set
 * @param value 要设置的值 Value to set
 * @return 操作是否成功 Whether the operation succeeded
 * 
 * 该函数用于在Redis数据库中设置键值对。首先检查Redis连接是否有效，
 * 然后执行SET命令，最后检查命令执行结果并返回操作状态。
 * 
 * This function is used to set key-value pairs in Redis database. It first checks if Redis connection is valid,
 * then executes SET command, and finally checks command execution result and returns operation status.
 */
bool RdConnector::set(const std::string& key, const std::string& value) {
    // 检查Redis上下文是否初始化，即是否已连接到Redis服务器
    // Check if Redis context is initialized, i.e., whether connected to Redis server
    if (context == nullptr){
        std::cerr<<"未连接"<<std::endl;
        return false;
    }
    // 执行Redis SET命令，将键值对存入Redis
    // Execute Redis SET command to store key-value pair in Redis
    auto* reply = (redisReply*)redisCommand(context, "SET %s %s", key.c_str(), value.c_str());
    // 检查Redis命令是否执行成功
    // Check if Redis command executed successfully
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "命令执行失败：" << (reply ? reply->str : context->errstr) << std::endl;
        freeReplyObject(reply);
        return false;
    }
    // 检查Redis返回的状态是否为"OK"，确认SET操作是否成功
    // Check if Redis returned status is "OK" to confirm SET operation succeeded
    bool success = (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    if (!success) {
        std::cerr << "SET 命令失败：" << (reply->str ? reply->str : "未知错误") << std::endl;
    }
    // 释放Redis回复对象，防止内存泄漏
    // Free Redis reply object to prevent memory leak
    freeReplyObject(reply);
    return success;
}



/**
 * @brief 设置Redis键值对并指定过期时间
 * Set Redis key-value pair with expiration time
 * @param key 要设置的键 Key to set
 * @param value 要设置的值 Value to set
 * @param expireSeconds 过期时间（秒） Expiration time in seconds
 * @return 操作是否成功 Whether the operation succeeded
 * 
 * 该函数用于在Redis数据库中设置键值对，并设置过期时间。首先检查Redis连接是否有效，
 * 然后执行SETEX命令（SET with EXpire），最后检查命令执行结果并返回操作状态。
 * 
 * This function is used to set key-value pairs in Redis database with expiration time. It first checks if Redis connection is valid,
 * then executes SETEX command (SET with EXpire), and finally checks command execution result and returns operation status.
 */
bool RdConnector::set(const std::string& key, const std::string& value, int expireSeconds) {
    // 检查Redis上下文是否初始化，即是否已连接到Redis服务器
    // Check if Redis context is initialized, i.e., whether connected to Redis server
    if (context == nullptr){
        std::cerr<<"未连接"<<std::endl;
        return false;
    }
    // 检查过期时间是否有效
    // Check if expiration time is valid
    if (expireSeconds <= 0) {
        std::cerr << "过期时间必须大于0" << std::endl;
        return false;
    }
    // 执行Redis SETEX命令，将键值对存入Redis并设置过期时间
    // Execute Redis SETEX command to store key-value pair in Redis with expiration time
    auto* reply = (redisReply*)redisCommand(context, "SETEX %s %d %s", key.c_str(), expireSeconds, value.c_str());
    // 检查Redis命令是否执行成功
    // Check if Redis command executed successfully
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "命令执行失败：" << (reply ? reply->str : context->errstr) << std::endl;
        freeReplyObject(reply);
        return false;
    }
    // 检查Redis返回的状态是否为"OK"，确认SETEX操作是否成功
    // Check if Redis returned status is "OK" to confirm SETEX operation succeeded
    bool success = (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    if (!success) {
        std::cerr << "SETEX 命令失败：" << (reply->str ? reply->str : "未知错误") << std::endl;
    }
    // 释放Redis回复对象，防止内存泄漏
    // Free Redis reply object to prevent memory leak
    freeReplyObject(reply);
    return success;
}

/**
 * @brief 检查Redis中指定键是否存在
 * Check if specified key exists in Redis
 * @param key 要检查的键名 Key name to check
 * @return 键存在返回true，不存在或发生错误返回false
 * Returns true if key exists, false if not exist or error occurs
 * 
 * 该函数用于检查Redis数据库中是否存在指定的键。首先检查Redis连接是否有效，
 * 然后执行EXISTS命令，最后根据返回结果判断键是否存在。
 * 
 * This function is used to check if a specified key exists in Redis database. It first checks if Redis connection is valid,
 * then executes EXISTS command, and finally judges whether the key exists based on the return result.
 */
bool RdConnector::exists(const std::string& key) {
    // 检查Redis上下文是否初始化，即是否已连接到Redis服务器
    // Check if Redis context is initialized, i.e., whether connected to Redis server
    if (context == nullptr){
        std::cerr<<"未连接"<<std::endl;
        return false;
    }
    // 执行Redis EXISTS命令检查键是否存在
    // Execute Redis EXISTS command to check if key exists
    auto* reply = (redisReply*)redisCommand(context, "EXISTS %s", key.c_str());
    // 检查Redis命令是否执行成功
    // Check if Redis command executed successfully
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "命令执行失败：" << (reply ? reply->str : context->errstr) << std::endl;
        freeReplyObject(reply);
        return false;
    }
    // EXISTS命令返回整数：1表示键存在，0表示键不存在
    // EXISTS command returns integer: 1 means key exists, 0 means key does not exist
    bool keyExists = false;
    if (reply->type == REDIS_REPLY_INTEGER) {
        keyExists = (reply->integer == 1);
    } else {
        std::cerr << "EXISTS 命令返回异常（类型：" << reply->type << "）" << std::endl;
    }
    // 释放Redis回复对象，防止内存泄漏
    // Free Redis reply object to prevent memory leak
    freeReplyObject(reply);
    return keyExists;
}

/**
 * @brief 获取RdConnector类的单例实例
 * Get singleton instance of RdConnector class
 * @return RdConnector类的单例实例，如果未初始化则返回nullptr
 * Singleton instance of RdConnector class, returns nullptr if not initialized
 */
RdConnector *RdConnector::getInstance() {
    return instance_;  // 返回静态单例实例指针 Return static singleton instance pointer
}

/**
 * @brief 获取连接错误信息
 * Get connection error message
 * @return 错误信息字符串，如果上下文为空则返回"未初始化连接"
 * Error message string, returns "未初始化连接" if context is null
 */
std::string RdConnector::getError() {
    if (context == nullptr) {
        return "未初始化连接";  // 连接未初始化时的错误信息 Error message when connection is not initialized
    }
    return context->errstr;  // 返回Redis上下文的错误信息 Return error message from Redis context
}

/**
 * @brief 删除Redis中的指定键
 * Delete specified key from Redis
 * @param key 要删除的键名 Key name to delete
 * @return 操作是否成功（键不存在视为成功）
 * Whether the operation succeeded (key not existing is considered successful)
 */
bool RdConnector::del(const std::string& key) {
    // 检查Redis连接是否有效
    // Check if Redis connection is valid
    if (context == nullptr) {
        std::cerr << "未连接Redis服务器" << std::endl;
        return false;
    }

    // 执行Redis DEL命令删除键
    // Execute Redis DEL command to delete key
    redisReply* reply = (redisReply*)redisCommand(context, "DEL %s", key.c_str());
    if (reply == nullptr) {
        std::cerr << "DEL命令执行失败：" << context->errstr << std::endl;
        return false;
    }

    // 处理命令返回结果
    // Process command return result
    bool success = true;
    if (reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "DEL命令失败：" << reply->str << std::endl;
        success = false;
    } else if (reply->type != REDIS_REPLY_INTEGER) {
        std::cerr << "DEL命令返回异常（类型：" << reply->type << "）" << std::endl;
        success = false;
    }

    // 释放Redis回复对象
    // Free Redis reply object
    freeReplyObject(reply);
    return success;
}
