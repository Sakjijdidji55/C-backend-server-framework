//
// Created by HP on 2025/11/5.
//

#include "../include/route.h"
#include <iostream>
#include <sstream>

void registerRoutes(Server &app) {
    // GET / - 首页
    app.get("/", [](const Request& /*req*/, Response& res) {
        res.json(R"({"message":"Welcome to C++ Server"})");
    });
}