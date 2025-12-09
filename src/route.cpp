//
// Created by HP on 2025/11/5.
// Route registration implementation file

#include "../include/route.h"
#include <iostream>
#include <sstream>

void registerRoutes(Server &app) {
    // 示例 Example
    // GET / - 首页
    // GET / - Home page
    app.get("/", [](const Request& /*req*/, Response& res) {
//        req.show();
        res.json(R"({"message":"Welcome to C++ Server"})");
    });

    app.post("/", [](const Request& req, Response& res) {
        req.show();
        res.success(req.bodyParams);
    });
}