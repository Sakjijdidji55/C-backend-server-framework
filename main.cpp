/**
 * @file main.cpp
 * @brief 服务器应用程序入口文件
 * @brief Server Application Entry Point
 * 
 * 此文件包含HTTP服务器的主入口函数，负责初始化数据库连接、创建服务器实例、
 * 注册路由以及启动服务器。
 * This file contains the main entry function of the HTTP server, responsible for initializing database connections,
 * creating server instances, registering routes, and starting the server.
 */
#include <iostream>
#include "include/Server.h"
#include "include/route.h"
#include "include/DBConnector.h"
//#include <hiredis/hiredis.h>
#include "RDConnector.h"
/**
 * @brief 程序主入口函数
 * @brief Main entry function of the program
 * @return 程序退出状态码
 * @return Program exit status code
 * @note 此函数是整个服务器应用程序的启动点，按顺序执行以下操作：
 *       1. 设置控制台编码（Windows系统）
 *       2. 初始化数据库连接
 *       3. 创建并配置HTTP服务器
 *       4. 注册路由处理函数
 *       5. 启动服务器监听
 * @note This function is the starting point of the entire server application, performing the following operations in order:
 *       1. Set console encoding (Windows system)
 *       2. Initialize database connection
 *       3. Create and configure HTTP server
 *       4. Register route handlers
 *       5. Start server listening
 */
int main() {
    #ifdef _WIN32
        // 设置控制台输出编码为 UTF-8（Windows 特定）
        // Set console output encoding to UTF-8 (Windows specific)
        SetConsoleOutputCP(65001);
        // 同时设置输入编码（可选）
        // Also set input encoding (optional)
        SetConsoleCP(65001);
    #endif

    // 初始化数据库连接器单例实例
    // Initialize database connector singleton instance
//    DBConnector::initInstance(
//            "localhost",    // 主机 Host
//            "root",         // 用户名 Username
//            "password",     // 替换为实际密码 Replace with actual password
//            "name", // 数据库名 Database name
//            3306            // 端口 Port
//    );
//
//    DBConnector::getInstance()->execute("SELECT * FROM users");

    // 创建服务器实例，指定监听端口8080
    // Create server instance, specifying port 8080 for listening
    Server app(std::stoi("8080"));

    // 注册所有路由处理函数
    // Register all route handlers
    registerRoutes(app);

    // 启动服务器，开始监听并处理HTTP请求
    // Start the server, begin listening for and handling HTTP requests
    app.run();
    
    // 服务器正常退出
    // Server exits normally
    return 0;
}
