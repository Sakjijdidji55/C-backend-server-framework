//
// Created by HP on 2025/12/9.
//

#include "Model.h"
#include "DBConnector.h"
#include "Log.h"
#include <utility>
#include <vector>

void Model::setTableDefault(const std::string& key, std::string value) {
    TableCols[key] = std::move(value);
}

/**
 * @brief 验证标识符（表名、字段名）是否安全
 * @param identifier 要验证的标识符
 * @return 如果标识符只包含字母、数字、下划线和反引号，返回true；否则返回false
 */
static bool isValidIdentifier(const std::string& identifier) {
    if (identifier.empty()) {
        return false;
    }
    for (char ch : identifier) {
        // 允许字母、数字、下划线、反引号（用于转义关键字）
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
 */
static std::string escapeIdentifier(const std::string& identifier) {
    // 如果已经包含反引号，说明已经转义过，直接返回
    if (!identifier.empty() && identifier.front() == '`' && identifier.back() == '`') {
        return identifier;
    }
    // 验证标识符是否安全
    if (!isValidIdentifier(identifier)) {
        std::cerr << "Warning: Invalid identifier detected: " << identifier << std::endl;
        // 对于不安全的标识符，用反引号转义（MySQL支持）
        return "`" + identifier + "`";
    }
    // 对于MySQL关键字，需要用反引号转义
    // 常见关键字列表（部分）
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
 * @brief 初始化数据库连接的函数
 *
 * 该函数用于建立与数据库的连接，并进行必要的初始化设置。
 * 包括但不限于：连接参数配置、连接池初始化、数据库表结构检查等。
 *
 * @note 该函数应在程序启动时尽早调用，以确保后续数据库操作可用
 * @warning 调用此函数前请确保相关配置文件已正确设置
 */
void Model::initDatabase() {
    try {
        // 检查表名是否为空，如果为空则输出错误信息并返回
        if (TableName.empty()) {
            std::cerr << "init database failed: TableName is empty" << std::endl;
            return;
        }
        // 检查表字段是否为空，如果为空则输出错误信息并返回
        if (TableCols.empty()) {
            std::cerr << "init database failed: TableCols is empty (no columns defined)" << std::endl;
            return;
        }

        // 验证并转义表名
        std::string safeTableName = escapeIdentifier(TableName);

        // 确保字符集有默认值（推荐 utf8mb4 以支持 emoji 和全量 Unicode）
        std::string usedCharset = charset.empty() ? "utf8mb4" : charset;
        // 验证字符集名称
        if (usedCharset != "utf8mb4" && usedCharset != "utf8" && usedCharset != "latin1") {
            std::cerr << "Warning: Unusual charset specified: " << usedCharset << ", using utf8mb4" << std::endl;
            usedCharset = "utf8mb4";
        }

        // 创建表的基础SQL语句
        std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + safeTableName + " (";

        // 拼接字段列表，处理末尾逗号问题
        size_t colCount = 0;  // 用于计数字段数量
        for (const auto &[key, value]: TableCols) {
            if (colCount > 0) {
                createTableSql += ", "; // 仅在非第一个字段前加逗号
            }
            // 转义字段名
            std::string safeKey = escapeIdentifier(key);
            createTableSql += safeKey + " " + value;  // 拼接字段名和字段类型
            colCount++;
        }

        // 补充表级设置（存储引擎、字符集）
        createTableSql += ") ENGINE=InnoDB DEFAULT CHARSET=" + usedCharset + ";";

        // 执行 SQL 并输出详细错误信息
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