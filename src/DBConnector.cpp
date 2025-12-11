/**
 * @file DBConnector.cpp
 * @brief 数据库连接器实现文件
 * @brief Database Connector Implementation File
 * 
 * 此文件实现了MySQL数据库连接器，采用单例模式设计，
 * 提供数据库连接、查询执行、结果处理等功能。
 * This file implements the MySQL database connector, designed with singleton pattern,
 * providing database connection, query execution, result processing and other functions.
 */
//
// Created by HP on 2025/12/9.
//
#include "../include/DBConnector.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

/**
 * @brief 静态成员初始化
 * @brief Static member initialization
 */
DBConnector* DBConnector::instance_ = nullptr;
std::mutex DBConnector::mutex_;

/**
 * @brief 私有构造函数
 * @param host 数据库主机地址
 * @param user 数据库用户名
 * @param passwd 数据库密码
 * @param dbname 数据库名称
 * @param port 数据库端口
 * @brief Private constructor
 * @param host Database host address
 * @param user Database username
 * @param passwd Database password
 * @param dbname Database name
 * @param port Database port
 * @note 构造函数初始化MySQL句柄
 * @note Constructor initializes MySQL handle
 * @exception std::runtime_error 当MySQL句柄初始化失败时抛出异常
 * @exception std::runtime_error Throws exception when MySQL handle initialization fails
 */
DBConnector::DBConnector(std::string host, std::string user,
                         std::string passwd, std::string dbname,
                         unsigned int port)
        : db_(nullptr), host_(std::move(host)),
          user_(std::move(user)), passwd_(std::move(passwd)),
          dbname_(std::move(dbname)), port_(port) {
    // 初始化MySQL句柄
    // Initialize MySQL handle
    db_ = mysql_init(nullptr);
    if (db_ == nullptr) {
        last_error_ = "MySQL句柄初始化失败";
        throw std::runtime_error(last_error_);
    }
}

/**
 * @brief 析构函数
 * @brief Destructor
 * @note 关闭数据库连接并释放资源
 * @note Closes database connection and releases resources
 */
DBConnector::~DBConnector() {
    if (db_ != nullptr) {
        mysql_close(db_);
        db_ = nullptr;
    }
    last_error_.clear();
}

/**
 * @brief 释放结果集
 * @param res MySQL结果集指针
 * @brief Free result set
 * @param res MySQL result set pointer
 * @note 安全释放MySQL查询结果集资源
 * @note Safely releases MySQL query result set resources
 */
void DBConnector::freeResult(MYSQL_RES* res) {
    if (res != nullptr) {
        mysql_free_result(res);
    }
}

/**
 * @brief 连接数据库
 * @return 连接是否成功
 * @brief Connect to database
 * @return Whether the connection was successful
 * @note 建立与MySQL数据库的连接，设置utf8mb4字符集避免中文乱码
 * @note Establishes connection with MySQL database, sets utf8mb4 charset to avoid Chinese garbled characters
 * @warning 连接失败时会设置错误信息，可通过getError()获取
 * @warning Error information is set on connection failure, can be obtained via getError()
 */
bool DBConnector::connect() {
    if (db_ == nullptr) {
        last_error_ = "MySQL句柄未初始化";
        return false;
    }

    // 设置字符集（避免中文乱码）
    // Set character set (avoid Chinese garbled characters)
    mysql_options(db_, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // 建立连接
    // Establish connection
    if (!mysql_real_connect(db_, host_.c_str(), user_.c_str(),
                            passwd_.c_str(), dbname_.c_str(),
                            port_, nullptr, 0)) {
        last_error_ = std::string("连接失败: ") + mysql_error(db_);
        return false;
    }

    last_error_.clear();
    return true;
}

/**
 * @brief 执行查询（SELECT）
 * @param sql SQL查询语句
 * @return 查询结果集，每行数据为字段名到值的映射
 * @brief Execute query (SELECT)
 * @param sql SQL query statement
 * @return Query result set, each row is a map from field name to value
 * @note 执行SELECT类型的SQL语句并返回结果集
 * @note Executes SELECT type SQL statements and returns result set
 * @warning 查询失败时会设置错误信息，可通过getError()获取
 * @warning Error information is set on query failure, can be obtained via getError()
 */
std::vector<std::map<std::string, std::string>> DBConnector::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> result;
    if (db_ == nullptr) {
        last_error_ = "未连接数据库";
        return result;
    }

    // 执行SQL
    // Execute SQL
    if (mysql_query(db_, sql.c_str()) != 0) {
        last_error_ = std::string("SQL执行失败: ") + mysql_error(db_);
        return result;
    }

    // 获取结果集
    // Get result set
    MYSQL_RES* res = mysql_store_result(db_);
    if (res == nullptr) {
        last_error_ = std::string("获取结果集失败: ") + mysql_error(db_);
        return result;
    }

    // 获取字段信息
    // Get field information
    int field_count = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    if (fields == nullptr || field_count <= 0) {
        freeResult(res);
        last_error_ = "无字段信息";
        return result;
    }

    // 遍历行数据
    // Iterate over row data
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        std::map<std::string, std::string> row_map;
        // 遍历字段，填充键值对（字段名->值）
        // Iterate over fields, fill key-value pairs (field name->value)
        for (int i = 0; i < field_count; ++i) {
            std::string field_name = fields[i].name;
            std::string field_value = row[i] ? row[i] : ""; // NULL值存为空字符串
            row_map[std::move(field_name)] = std::move(field_value);
        }
        result.push_back(std::move(row_map));
    }

    // 释放资源
    // Release resources
    freeResult(res);
    last_error_.clear();
    return result;
}

/**
 * @brief 执行命令（INSERT/UPDATE/DELETE）
 * @param sql SQL命令语句
 * @return 受影响的行数，执行失败返回-1
 * @brief Execute command (INSERT/UPDATE/DELETE)
 * @param sql SQL command statement
 * @return Number of affected rows, returns -1 on execution failure
 * @note 执行非查询类型的SQL语句（INSERT/UPDATE/DELETE）
 * @note Executes non-query type SQL statements (INSERT/UPDATE/DELETE)
 * @warning 执行失败时会设置错误信息，可通过getError()获取
 * @warning Error information is set on execution failure, can be obtained via getError()
 */
int DBConnector::execute(const std::string& sql) {
    if (db_ == nullptr) {
        last_error_ = "未连接数据库";
        return -1;
    }

    // 执行SQL
    // Execute SQL
    if (mysql_query(db_, sql.c_str()) != 0) {
        last_error_ = std::string("SQL执行失败: ") + mysql_error(db_);
        return -1;
    }

    // 获取影响行数
    // Get affected rows
    int affected_rows = mysql_affected_rows(db_);
    last_error_.clear();
    return affected_rows;
}

// 获取最后一次错误信息
std::string DBConnector::getError() {
    return last_error_;
}

// 初始化单例（必须先调用）
void DBConnector::initInstance(const std::string& host, const std::string& user,
                               const std::string& passwd, const std::string& dbname,
                               unsigned int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr) {
        try {
            instance_ = new DBConnector(host, user, passwd, dbname, port);
        } catch (const std::runtime_error& e) {
            std::cerr << "单例初始化失败: " << e.what() << std::endl;
            instance_ = nullptr;
        }
    }
}

// 获取单例实例
DBConnector* DBConnector::getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr) {
        throw std::runtime_error("单例未初始化，请先调用initInstance");
    }
    return instance_;
}

// 销毁单例
void DBConnector::destroyInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
    }
}

// SQL注入防护：转义SQL字符串字面量
std::string DBConnector::escapeSqlLiteral(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() * 2); // 预留空间以避免多次重新分配
    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\'':
                escaped += "''";
                break;
            case '\0':
                escaped += "\\0";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\x1A':
                escaped += "\\Z";
                break;
            default:
                escaped.push_back(ch);
        }
    }
    return escaped;
}