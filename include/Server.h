#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <map>
#include <vector>
#include <sstream>
#include <cstdio>
#include "JsonValue.h"


#ifdef _WIN32
#include <winsock2.h>    // Windows平台网络头文件
#include <windows.h>
#else
#include <sys/socket.h>  // Linux/Unix平台网络头文件
#include <netinet/in.h>  // 包含sockaddr_in定义
#include <arpa/inet.h>
#endif

#include "Threadpool.h"

#include "Log.h"

std::string mpToJson(const std::map<std::string, std::string>& mp);

std::string getFormattedDate();

// 请求结构
// Request structure
struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> queryParams;  // 查询参数
    std::map<std::string, std::string> bodyParams;   // 表单参数

    [[nodiscard]] std::string query_param(const std::string& key) const {
        return queryParams.count(key) ? queryParams.at(key) : "";
    }

    void show() const {
        std::cout<<"Method: " + method + "\n";
        std::cout<<"Path: " + path + "\n";
        for (const auto& [key, value] : headers) {
            std::cout<<key + ": " + value + "\n";
        }
        std::cout<<"Body: " + body + "\n";
    }

    void parseBody() {
        std::istringstream iss(body);
        std::string line;
        while(std::getline(iss, line)) {
            size_t pos=line.find(':');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos+1);
                bodyParams[key] = value;
            }
        }
    }
};

// 响应结构
// Response structure
struct Response {
    int statusCode = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    Response() {
        headers["Content-Type"] = "application/json; charset=utf-8";
    }
    
    // 便捷方法：设置JSON响应
    void json(const std::string& jsonStr) {
        body = jsonStr;
        headers["Content-Type"] = "application/json; charset=utf-8";
    }
    
    // 便捷方法：设置文本响应
    void text(const std::string& textStr) {
        body = textStr;
        headers["Content-Type"] = "text/plain; charset=utf-8";
    }

    // 便捷方法：设置HTML响应
    void status(int code) {
        statusCode = code;
    }

    void success(std::map<std::string, std::string>& resMap) {
        resMap["status"] = "ok";
        resMap["message"] = "Success";
        json(mpToJson(resMap));
    }

    // 新版：支持嵌套JSON结构的success方法
    void success(const std::map<std::string, JsonValue>& resMap) {
        std::map<std::string, JsonValue> result;
        result["status"] = JsonValue("ok");
        result["message"] = JsonValue("Success");
        // 合并传入的数据
        for (const auto& pair : resMap) {
            result[pair.first] = pair.second;
        }
        json(toJson(result));
    }

    void success() {
        json(R"({"status":"ok", "message":"Success"})");
    }

    void error(int code, const std::string& message) {
        Log::getInstance()->write("Time "+getFormattedDate()+" Code "+std::to_string(code)+" Error: " + message + "\n");
        statusCode = code;
        json(R"({"status":"fail", "message":")" + message + "\"}");
    }
};

// 路由处理器类型
typedef std::function<void(const Request&, Response&)> Handler;

// 简单的HTTP服务器类
// Simple HTTP server class
class Server {
public:
    // 构造函数，初始化服务器端口
    // Constructor, initialize server port
    explicit Server(int port = 8080);
    // 析构函数
    // Destructor
    ~Server();
    
    // 注册路由 - GET方法
    // Register route - GET method
    void get(const std::string& path, Handler handler);
    // 注册路由 - POST方法
    // Register route - POST method
    void post(const std::string& path, Handler handler);
    // 注册路由 - PUT方法
    // Register route - PUT method
    void put(const std::string& path, Handler handler);
    // 注册路由 - DELETE方法
    // Register route - DELETE method
    void del(const std::string& path, Handler handler);
    
    // 启动服务器
    // Start server
    void run();
    
    // 停止服务器
    // Stop server
    void stop();

    // 获取服务器实例（单例模式）
    // Get server instance (singleton pattern)
    static Server* getInstance();
private:
    int port_;
    int serverSocket_;
    bool running_;
    std::mutex routesMutex_;
    std::mutex logMutex_;
    ThreadPool threadpool_;
    
    // 路由表：method -> (path -> handler)
    std::map<std::string, std::map<std::string, Handler>> routes_;
    
    // 静态成员用于信号处理
    static Server* instance_;
    
    // 静态信号处理函数
    static void signalHandler(int sig);
    
    // 注册信号处理器
    static void registerSignalHandlers();
    
#ifdef _WIN32
    // Windows控制台关闭事件处理函数（友元函数）
    friend BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);
#endif
    
    // 内部方法
    void handleClient(int clientSocket, const sockaddr_in *clientAddress);
    static Request parseRequest(const std::string& requestStr);
    static std::string buildResponse(const Response& response);
    Handler findHandler(const std::string& method, const std::string& path);
    static std::map<std::string, std::string> parseQueryParams(const std::string& query);
    
    // 工具函数
    void printRegisteredRoutes() const;
    static std::string getClientIP(const sockaddr_in *clientAddress) ;

    static std::string urlDecode(const std::string &value);
};

#endif // SERVER_H

