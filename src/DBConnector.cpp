//
// Created by HP on 2025/12/9.
//
#include "../include/DBConnector.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

// 静态成员初始化
DBConnector* DBConnector::instance_ = nullptr;
std::mutex DBConnector::mutex_;

// 私有构造函数
DBConnector::DBConnector(std::string host, std::string user,
                         std::string passwd, std::string dbname,
                         unsigned int port)
        : db_(nullptr), host_(std::move(host)),
          user_(std::move(user)), passwd_(std::move(passwd)),
          dbname_(std::move(dbname)), port_(port), last_error_("") {
    // 初始化MySQL句柄
    db_ = mysql_init(nullptr);
    if (db_ == nullptr) {
        last_error_ = "MySQL句柄初始化失败";
        throw std::runtime_error(last_error_);
    }
}

// 析构函数
DBConnector::~DBConnector() {
    if (db_ != nullptr) {
        mysql_close(db_);
        db_ = nullptr;
    }
    last_error_.clear();
}

// 释放结果集
void DBConnector::freeResult(MYSQL_RES* res) {
    if (res != nullptr) {
        mysql_free_result(res);
    }
}

// 连接数据库
bool DBConnector::connect() {
    if (db_ == nullptr) {
        last_error_ = "MySQL句柄未初始化";
        return false;
    }

    // 设置字符集（避免中文乱码）
    mysql_options(db_, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // 建立连接
    if (!mysql_real_connect(db_, host_.c_str(), user_.c_str(),
                            passwd_.c_str(), dbname_.c_str(),
                            port_, nullptr, 0)) {
        last_error_ = std::string("连接失败: ") + mysql_error(db_);
        return false;
    }

    last_error_.clear();
    return true;
}

// 执行查询（SELECT）
std::vector<std::map<std::string, std::string>> DBConnector::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> result;
    if (db_ == nullptr) {
        last_error_ = "未连接数据库";
        return result;
    }

    // 执行SQL
    if (mysql_query(db_, sql.c_str()) != 0) {
        last_error_ = std::string("SQL执行失败: ") + mysql_error(db_);
        return result;
    }

    // 获取结果集
    MYSQL_RES* res = mysql_store_result(db_);
    if (res == nullptr) {
        last_error_ = std::string("获取结果集失败: ") + mysql_error(db_);
        return result;
    }

    // 获取字段信息
    int field_count = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    if (fields == nullptr || field_count <= 0) {
        freeResult(res);
        last_error_ = "无字段信息";
        return result;
    }

    // 遍历行数据
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        std::map<std::string, std::string> row_map;
        // 遍历字段，填充键值对（字段名->值）
        for (int i = 0; i < field_count; ++i) {
            std::string field_name = fields[i].name;
            std::string field_value = row[i] ? row[i] : ""; // NULL值存为空字符串
            row_map[std::move(field_name)] = std::move(field_value);
        }
        result.push_back(std::move(row_map));
    }

    // 释放资源
    freeResult(res);
    last_error_.clear();
    return result;
}

// 执行命令（INSERT/UPDATE/DELETE）
int DBConnector::execute(const std::string& sql) {
    if (db_ == nullptr) {
        last_error_ = "未连接数据库";
        return -1;
    }

    // 执行SQL
    if (mysql_query(db_, sql.c_str()) != 0) {
        last_error_ = std::string("SQL执行失败: ") + mysql_error(db_);
        return -1;
    }

    // 获取影响行数
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