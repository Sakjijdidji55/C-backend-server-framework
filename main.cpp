#include <iostream>
#include "include/Server.h"
#include "include/route.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    // 创建服务器
    Server app(std::stoi("8080"));
    registerRoutes(app);

    // 启动服务器
    app.run();
    return 0;
}
