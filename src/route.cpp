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

    app.post("/", [](const Request& /*req*/, Response& res) {
//        req.show();
        res.success();
    });

    // 下载接口模版：整文件下载（小文件，一次性读入内存）
    app.get("/download", [](const Request& req, Response& res) {
        std::string path = req.query_param("path");
        if (path.empty()) {
            res.error(400, "Missing query parameter: path");
            return;
        }
        res.file(path, "application/octet-stream", true, "");
    });

    // 下载接口模版：流式下载（大文件，分块发送，节省内存）
    app.get("/download/stream", [](const Request& req, Response& res) {
        std::string path = req.query_param("path");
        if (path.empty()) {
            res.error(400, "Missing query parameter: path");
            return;
        }
        res.fileStream(path, "application/octet-stream", true, "", 65536);
    });
}