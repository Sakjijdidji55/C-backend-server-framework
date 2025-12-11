/**
 * @file DBConnector.h
 * @brief 数据库连接工具类头文件
 * @brief Database Connector Utility Class Header File
 * @date 2025/12/9
 */

#ifndef CBSF_DBCONNECTOR_H
#define CBSF_DBCONNECTOR_H

#include <mysql.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>

/**
 * @class DBConnector
 * @brief 数据库连接工具类（单例模式）
 * @brief Database Connection Utility Class (Singleton Pattern)
 * @note 提供MySQL数据库的连接、查询、执行等功能，采用线程安全的单例模式
 * @note Provides MySQL database connection, query, execution functions with thread-safe singleton pattern
 */
class DBConnector {
private:
    MYSQL* db_;                  ///< MySQL连接句柄 (MySQL connection handle)
    std::string host_;           ///< 数据库主机地址 (Database host address)
    std::string user_;           ///< 数据库用户名 (Database username)
    std::string passwd_;         ///< 数据库密码 (Database password)
    std::string dbname_;         ///< 数据库名称 (Database name)
    unsigned int port_;          ///< 数据库端口号 (Database port number)
    std::string last_error_;     ///< 最后一次错误信息 (Last error message)
    static DBConnector* instance_; ///< 单例实例指针 (Singleton instance pointer)
    static std::mutex mutex_;    ///< 线程安全锁 (Thread safety lock)

    /**
     * @brief 私有构造函数（单例模式，禁止外部实例化）
     * @brief Private constructor (Singleton pattern, prevents external instantiation)
     * @param host 数据库主机地址
     * @param user 用户名
     * @param passwd 密码
     * @param dbname 数据库名
     * @param port 端口号，默认3306
     */
    DBConnector(std::string host, std::string user,
                std::string passwd, std::string dbname,
                unsigned int port = 3306);

    /**
     * @brief 禁用拷贝构造函数
     * @brief Disable copy constructor
     */
    DBConnector(const DBConnector&) = delete;
    
    /**
     * @brief 禁用赋值运算符
     * @brief Disable assignment operator
     */
    DBConnector& operator=(const DBConnector&) = delete;

    /**
     * @brief 释放结果集（内部工具函数）
     * @brief Free result set (internal utility function)
     * @param res 待释放的结果集指针
     */
    static void freeResult(MYSQL_RES* res);

public:
    /**
     * @brief 析构函数（关闭连接）
     * @brief Destructor (closes connection)
     */
    ~DBConnector();

    /**
     * @brief 连接数据库
     * @brief Connect to database
     * @return 连接成功返回true，失败返回false
     * @return Returns true if connection succeeds, false otherwise
     */
    bool connect();

    /**
     * @brief 执行SQL查询（适合SELECT，返回二维键值对：行->字段名-值）
     * @brief Execute SQL query (suitable for SELECT, returns 2D key-value pairs: row->field-value)
     * @param sql SQL查询语句
     * @return 查询结果集
     */
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);

    /**
     * @brief 执行SQL命令（适合INSERT/UPDATE/DELETE，返回影响行数）
     * @brief Execute SQL command (suitable for INSERT/UPDATE/DELETE, returns affected rows count)
     * @param sql SQL命令语句
     * @return 受影响的行数，执行失败返回-1
     * @return Number of affected rows, returns -1 on failure
     */
    int execute(const std::string& sql);

    /**
     * @brief 获取最后一次错误信息
     * @brief Get last error message
     * @return 错误信息字符串
     */
    std::string getError();

    /**
     * @brief 初始化单例（必须先调用此函数初始化参数）
     * @brief Initialize singleton (must call this function first to initialize parameters)
     * @param host 数据库主机地址
     * @param user 用户名
     * @param passwd 密码
     * @param dbname 数据库名
     * @param port 端口号，默认3306
     */
    static void initInstance(const std::string& host, const std::string& user,
                             const std::string& passwd, const std::string& dbname,
                             unsigned int port = 3306);

    /**
     * @brief 获取单例实例（需先调用initInstance）
     * @brief Get singleton instance (must call initInstance first)
     * @return 单例实例指针，若未初始化返回nullptr
     * @return Singleton instance pointer, returns nullptr if not initialized
     */
    static DBConnector* getInstance();

    /**
     * @brief 销毁单例
     * @brief Destroy singleton
     */
    static void destroyInstance();

    /**
     * @brief 转义SQL字符串，防止SQL注入
     * @brief Escape SQL string to prevent SQL injection
     * @param value 待转义的字符串
     * @return 转义后的字符串
     */
    static std::string escapeSqlLiteral(const std::string &value);
};

#endif //CBSF_DBCONNECTOR_H
