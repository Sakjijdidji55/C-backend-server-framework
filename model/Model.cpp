/**
 * @file Model.cpp
 * @brief 数据模型基类实现文件
 * @brief Base Model Class Implementation File
 * 
 * 此文件实现了Model类的方法，提供数据库操作的基础功能
 * This file implements the methods of the Model class, providing basic database operation functionality
 */
//
// Created by HP on 2025/12/9.
//

#include "Model.h"
#include "DBConnector.h"
#include "Log.h"
#include <utility>
#include <vector>

/**
 * @brief 设置表字段默认值
 * @param key 字段名
 * @param value 字段定义
 * @brief Set table field default value
 * @param key Field name
 * @param value Field definition
 * @note 此方法将字段名和字段定义添加到TableCols映射中
 * @note This method adds field name and field definition to the TableCols map
 */
void Model::setTableDefault(const std::string& key, std::string value) {
    TableCols[key] = std::move(value);
}

/**
 * @brief 验证标识符（表名、字段名）是否安全
 * @param identifier 要验证的标识符
 * @return 如果标识符只包含字母、数字、下划线和反引号，返回true；否则返回false
 * @brief Validate if an identifier (table name, column name) is safe
 * @param identifier The identifier to validate
 * @return Returns true if identifier contains only letters, numbers, underscores and backticks; false otherwise
 * @note 此函数用于防止SQL注入攻击，确保标识符符合安全标准
 * @note This function is used to prevent SQL injection attacks by ensuring identifiers meet security standards
 */
static bool isValidIdentifier(const std::string& identifier) {
    if (identifier.empty()) {
        return false;
    }
    for (char ch : identifier) {
        // 允许字母、数字、下划线、反引号（用于转义关键字）
        // Allow letters, numbers, underscores, and backticks (for escaping keywords)
        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_' || ch == '`')) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 转义标识符（表名、字段名）以防止SQL注入
 * @param identifier 要转义的标识符
 * @return 转义后的标识符（用反引号包围）
 * @brief Escape identifier (table name, column name) to prevent SQL injection
 * @param identifier The identifier to escape
 * @return The escaped identifier (enclosed in backticks if needed)
 * @note 此函数会自动处理关键字转义和不安全标识符的转义
 * @note This function automatically handles escaping for keywords and unsafe identifiers
 */
static std::string escapeIdentifier(const std::string& identifier) {
    // 如果已经包含反引号，说明已经转义过，直接返回
    // If already contains backticks, it's already escaped, return directly
    if (!identifier.empty() && identifier.front() == '`' && identifier.back() == '`') {
        return identifier;
    }
    // 验证标识符是否安全
    // Validate if the identifier is safe
    if (!isValidIdentifier(identifier)) {
        std::cerr << "Warning: Invalid identifier detected: " << identifier << std::endl;
        // 对于不安全的标识符，用反引号转义（MySQL支持）
        // For unsafe identifiers, escape with backticks (MySQL support)
        return "`" + identifier + "`";
    }
    // 对于MySQL关键字，需要用反引号转义
    // For MySQL keywords, need to escape with backticks
    // 常见关键字列表（部分）
    // Common keywords list (partial)
    static const std::vector<std::string> keywords = {
            "from", "to", "order", "group", "select", "insert", "update", "delete",
            "create", "drop", "table", "database", "index", "key", "primary", "foreign"
    };
    for (const auto& keyword : keywords) {
        if (identifier == keyword) {
            return "`" + identifier + "`";
        }
    }
    return identifier;
}

/**
 * @brief 初始化数据库表结构
 * @brief Initialize database table structure
 * @note 此函数用于创建数据库表（如果不存在），并设置表的字符集和存储引擎
 * @note This function is used to create database tables (if not exists) and set table charset and storage engine
 * @warning 调用此函数前请确保相关配置已正确设置
 * @warning Please ensure relevant configurations are properly set before calling this function
 */
void Model::initDatabase() {
    try {
        // 检查表名是否为空，如果为空则输出错误信息并返回
        // Check if table name is empty, output error message and return if empty
        if (TableName.empty()) {
            std::cerr << "init database failed: TableName is empty" << std::endl;
            return;
        }
        // 检查表字段是否为空，如果为空则输出错误信息并返回
        // Check if table columns are empty, output error message and return if empty
        if (TableCols.empty()) {
            std::cerr << "init database failed: TableCols is empty (no columns defined)" << std::endl;
            return;
        }

        // 验证并转义表名
        // Validate and escape table name
        std::string safeTableName = escapeIdentifier(TableName);

        // 确保字符集有默认值（推荐 utf8mb4 以支持 emoji 和全量 Unicode）
        // Ensure charset has a default value (utf8mb4 recommended for emoji and full Unicode support)
        std::string usedCharset = charset.empty() ? "utf8mb4" : charset;
        // 验证字符集名称
        // Validate charset name
        if (usedCharset != "utf8mb4" && usedCharset != "utf8" && usedCharset != "latin1") {
            std::cerr << "Warning: Unusual charset specified: " << usedCharset << ", using utf8mb4" << std::endl;
            usedCharset = "utf8mb4";
        }

        // 创建表的基础SQL语句
        // Base SQL statement for creating table
        std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + safeTableName + " (";

        // 拼接字段列表，处理末尾逗号问题
        // Concatenate column list, handle trailing comma issue
        size_t colCount = 0;  // 用于计数字段数量
                            // Used to count the number of fields
        for (const auto &[key, value]: TableCols) {
            if (colCount > 0) {
                createTableSql += ", "; // 仅在非第一个字段前加逗号
                                       // Add comma only before non-first fields
            }
            // 转义字段名
            // Escape column name
            std::string safeKey = escapeIdentifier(key);
            createTableSql += safeKey + " " + value;  // 拼接字段名和字段类型
                                                    // Concatenate column name and column type
            colCount++;
        }

        // 补充表级设置（存储引擎、字符集）
        // Add table-level settings (storage engine, charset)
        createTableSql += ") ENGINE=InnoDB DEFAULT CHARSET=" + usedCharset + ";";

        // 执行 SQL 并输出详细错误信息
        // Execute SQL and output detailed error information
        int executeCode = DBConnector::getInstance()->execute(createTableSql);
        if (executeCode == -1) {
            std::cerr << "init database failed: SQL execution error. SQL: " << createTableSql << std::endl;
            std::cerr << "Database error: " << DBConnector::getInstance()->getError() << std::endl;
        } else {
            std::cout << "init database success. Table: " << TableName << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "init database failed: " << e.what() << std::endl;
        Log::getInstance()->write("init database failed: " + std::string(e.what()));
    }
}