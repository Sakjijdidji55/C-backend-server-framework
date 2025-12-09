#include <iostream>
#include "include/Server.h"
#include "include/route.h"
#include "include/DBConnector.h"

int main() {
    #ifdef _WIN32
        // 设置控制台输出编码为 UTF-8（Windows 特定）
        SetConsoleOutputCP(65001);
        // 同时设置输入编码（可选）
        SetConsoleCP(65001);
    #endif

    DBConnector::initInstance(
            "localhost",    // 主机
            "root",         // 用户名
            "5define7eS",     // 替换为实际密码
            "flight_qt_server", // 数据库名
            3306            // 端口
    );

    // 创建服务器 Create server
    Server app(std::stoi("1437"));

    registerRoutes(app);

    // 启动服务器 Start server
    app.run();
    return 0;
}
