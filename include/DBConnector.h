//
// Created by HP on 2025/12/9.
//

#ifndef CBSF_DBCONNECTOR_H
#define CBSF_DBCONNECTOR_H

#include <mysql.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>

// 数据库连接工具类（单例模式）
class DBConnector {
private:
    MYSQL* db_;                  // MySQL连接句柄（注意：原代码是Mysql，改为MYSQL（大写），符合C API规范）
    std::string host_;           // 数据库主机
    std::string user_;           // 用户名
    std::string passwd_;         // 密码
    std::string dbname_;         // 数据库名
    unsigned int port_;          // 端口
    std::string last_error_;     // 最后一次错误信息
    static DBConnector* instance_; // 单例实例
    static std::mutex mutex_;    // 线程安全锁

    // 私有构造函数（单例模式，禁止外部实例化）
    DBConnector(std::string host, std::string user,
                std::string passwd, std::string dbname,
                unsigned int port = 3306);

    // 禁用拷贝构造和赋值运算符
    DBConnector(const DBConnector&) = delete;
    DBConnector& operator=(const DBConnector&) = delete;

    // 释放结果集（内部工具函数）
    void freeResult(MYSQL_RES* res);

public:
    // 析构函数（关闭连接）
    ~DBConnector();

    // 连接数据库
    bool connect();

    // 执行SQL查询（适合SELECT，返回二维键值对：行->字段名-值）
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);

    // 执行SQL命令（适合INSERT/UPDATE/DELETE，返回影响行数）
    int execute(const std::string& sql);

    // 获取最后一次错误信息
    std::string getError();

    // 初始化单例（必须先调用此函数初始化参数）
    static void initInstance(const std::string& host, const std::string& user,
                             const std::string& passwd, const std::string& dbname,
                             unsigned int port = 3306);

    // 获取单例实例（需先调用initInstance）
    static DBConnector* getInstance();

    // 销毁单例
    static void destroyInstance();

    static std::string escapeSqlLiteral(const std::string &value);
};

#endif //CBSF_DBCONNECTOR_H
