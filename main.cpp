#include <iostream>
#include "include/Server.h"
#include "include/route.h"

int main() {
    // 创建服务器 Create server
    Server app(std::stoi("8080"));

    registerRoutes(app);

    // 启动服务器 Start server
    app.run();
    return 0;
}
