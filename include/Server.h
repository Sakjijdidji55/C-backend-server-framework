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

#include "JsonValue.h"

// 请求结构
// Request structure
struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> queryParams;  // 查询参数
    std::map<std::string, std::string> bodyParams;   // 表单参数或JSON参数
    std::unique_ptr<JsonValue> jsonBody;            // 解析后的JSON对象（如果适用）

    [[nodiscard]] std::string query_param(const std::string& key) const {
        auto it = queryParams.find(key);
        return it != queryParams.end() ? it->second : "";
    }

    void show() const {
        std::cout << "Method: " + method + "\n";
        std::cout << "Path: " + path + "\n";
        std::cout << "Query Parameters:\n";
        for (const auto& [key, value] : queryParams) {
            std::cout << "  " << key << " = " << value << "\n";
        }
        std::cout << "Headers:\n";
        for (const auto& [key, value] : headers) {
            std::cout << "  " << key << ": " << value << "\n";
        }
        std::cout << "Body: " << body << "\n";

        if (jsonBody) {
            std::cout << "Parsed JSON Body: " << jsonBody->toJson() << "\n";
        } else {
            std::cout << "Body Parameters (key-value):\n";
            for (const auto& [key, value] : bodyParams) {
                std::cout << "  " << key << " = " << value << "\n";
            }
        }
    }

    /**
     * 解析请求体，根据Content-Type处理不同格式
     */
    void parseBody() {
        if (body.empty()) {
            return;
        }

        // 获取Content-Type，不区分大小写
        std::string contentType;
        for (const auto& [key, value] : headers) {
            if (strcasecmp(key.c_str(), "content-type") == 0) {
                contentType = value;
                break;
            }
        }

        // 转换为小写以便比较
        std::transform(contentType.begin(), contentType.end(),
                       contentType.begin(), ::tolower);

        // 移除可能的字符集信息，如"application/x-www-form-urlencoded; charset=UTF-8"
        size_t semicolonPos = contentType.find(';');
        if (semicolonPos != std::string::npos) {
            contentType = contentType.substr(0, semicolonPos);
        }

        // 去除空格
        contentType.erase(std::remove_if(contentType.begin(), contentType.end(), ::isspace),
                          contentType.end());

        // 根据Content-Type调用相应的解析函数
        if (contentType == "application/x-www-form-urlencoded") {
            parseUrlEncodedBody();
        } else if (contentType == "application/json") {
            parseJsonBody();
        } else if (contentType == "multipart/form-data") {
            // 处理multipart/form-data（较复杂，需要边界）
            std::cerr << "Warning: multipart/form-data parsing not fully implemented\n";
            parseMultipartFormData();
        } else if (contentType == "text/plain") {
            // 纯文本，直接存储
            bodyParams["_raw_text"] = body;
        } else if (contentType.empty()) {
            // 没有Content-Type，尝试自动检测
            autoDetectContentType();
        } else {
            // 未知类型，作为原始文本处理
            std::cerr << "Warning: Unknown Content-Type: " << contentType << "\n";
            bodyParams["_raw_data"] = body;
        }
    }

    /**
     * 获取JSON请求体中的值
     * @param key JSON键
     * @return 如果键存在且是字符串类型，返回字符串值；否则返回空字符串
     */
    [[nodiscard]] std::string json_param(const std::string& key) const {
        if (!jsonBody) {
            return "";
        }

        try {
            if (jsonBody->getType() == JsonValue::OBJECT) {
                auto obj = jsonBody->asObject();
                auto it = obj.find(key);
                if (it != obj.end()) {
                    if (it->second.getType() == JsonValue::STRING) {
                        return it->second.asString();
                    } else if (it->second.getType() == JsonValue::NUMBER) {
                        return std::to_string(it->second.asNumber());
                    } else if (it->second.getType() == JsonValue::BOOLEAN) {
                        return it->second.asBoolean() ? "true" : "false";
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting JSON parameter: " << e.what() << "\n";
        }

        return "";
    }

    /**
     * 检查请求体是否是JSON
     */
    [[nodiscard]] bool isJson() const {
        return jsonBody != nullptr;
    }

    /**
     * 获取解析后的JSON对象
     */
    [[nodiscard]] const JsonValue* getJsonBody() const {
        return jsonBody.get();
    }

private:
    /**
     * 解析application/x-www-form-urlencoded格式的请求体
     * 格式: key1=value1&key2=value2
     */
    void parseUrlEncodedBody() {
        std::stringstream ss(body);
        std::string pair;

        while (std::getline(ss, pair, '&')) {
            if (pair.empty()) continue;

            size_t equalsPos = pair.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = pair.substr(0, equalsPos);
                std::string value = pair.substr(equalsPos + 1);

                // URL解码
                key = urlDecode(key);
                value = urlDecode(value);

                bodyParams[key] = value;
            } else {
                // 只有key没有value的情况
                bodyParams[urlDecode(pair)] = "";
            }
        }
    }

    /**
     * 解析JSON格式的请求体（使用JsonValue类）
     */
    void parseJsonBody() {
        try {
            // 创建JsonValue对象并解析JSON
            jsonBody = std::make_unique<JsonValue>();
            jsonBody->fromJson(body);

            // 如果是JSON对象，将其扁平化到bodyParams以便向后兼容
            if (jsonBody->getType() == JsonValue::OBJECT) {
                flattenJsonToParams(*jsonBody, "");
            } else if (jsonBody->getType() == JsonValue::ARRAY) {
                // 数组类型的JSON，存储到特殊键
                bodyParams["_json_array"] = jsonBody->toJson();
            } else {
                // 其他基本类型
                bodyParams["_json_value"] = jsonToString(*jsonBody);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON body: " << e.what() << "\n";
            // JSON解析失败，尝试作为普通文本处理
            bodyParams["_invalid_json"] = body;
        }
    }

    /**
     * 将JSON对象扁平化为键值对（用于向后兼容）
     * @param json JsonValue对象
     * @param prefix 键的前缀（用于嵌套对象）
     */
    void flattenJsonToParams(const JsonValue& json, const std::string& prefix) {
        if (json.getType() == JsonValue::OBJECT) {
            auto obj = json.asObject();
            for (const auto& [key, value] : obj) {
                std::string newKey = prefix.empty() ? key : prefix + "." + key;

                if (value.getType() == JsonValue::OBJECT) {
                    // 递归处理嵌套对象
                    flattenJsonToParams(value, newKey);
                } else if (value.getType() == JsonValue::ARRAY) {
                    // 数组存储为JSON字符串
                    bodyParams[newKey] = value.toJson();
                } else {
                    // 基本类型转换为字符串
                    bodyParams[newKey] = jsonToString(value);
                }
            }
        }
    }

    /**
     * 将JsonValue基本类型转换为字符串
     */
    static std::string jsonToString(const JsonValue& json) {
        switch (json.getType()) {
            case JsonValue::STRING:
                return json.asString();
            case JsonValue::NUMBER:
                return std::to_string(json.asNumber());
            case JsonValue::BOOLEAN:
                return json.asBoolean() ? "true" : "false";
            case JsonValue::NULL_TYPE:
                return "";
            default:
                return json.toJson();
        }
    }

    /**
     * 解析multipart/form-data（简化实现）
     */
    void parseMultipartFormData() {
        // 提取boundary
        std::string contentTypeHeader;
        for (const auto& [key, value] : headers) {
            if (strcasecmp(key.c_str(), "content-type") == 0) {
                contentTypeHeader = value;
                break;
            }
        }

        std::string boundary;
        size_t boundaryPos = contentTypeHeader.find("boundary=");
        if (boundaryPos != std::string::npos) {
            boundary = contentTypeHeader.substr(boundaryPos + 9);
            // 去除可能的引号
            if (!boundary.empty() && boundary[0] == '"') {
                boundary = boundary.substr(1, boundary.size() - 2);
            }
        }

        if (boundary.empty()) {
            std::cerr << "Error: No boundary found in multipart/form-data\n";
            return;
        }

        // 完整的multipart解析较复杂，这里只做简单示例
        std::string delimiter = "--" + boundary;
        std::string endDelimiter = delimiter + "--";

        size_t startPos = body.find(delimiter);
        if (startPos == std::string::npos) {
            return;
        }

        startPos += delimiter.length();
        if (body[startPos] == '\r') startPos++;
        if (body[startPos] == '\n') startPos++;

        while (true) {
            size_t nextDelimiter = body.find(delimiter, startPos);
            if (nextDelimiter == std::string::npos) {
                break;
            }

            std::string part = body.substr(startPos, nextDelimiter - startPos);
            parseMultipartPart(part);

            startPos = nextDelimiter + delimiter.length();
            if (body[startPos] == '\r') startPos++;
            if (body[startPos] == '\n') startPos++;

            // 检查是否是结束边界
            if (body.substr(startPos, 2) == "--") {
                break;
            }
        }
    }

    /**
     * 解析multipart的单个part
     */
    void parseMultipartPart(const std::string& part) {
        // 查找头部结束位置（空行）
        size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = part.find("\n\n");
            if (headerEnd == std::string::npos) {
                return;
            }
        }

        std::string headersStr = part.substr(0, headerEnd);
        std::string content = part.substr(headerEnd + 4); // 跳过空行

        // 解析Content-Disposition获取name
        std::string name;
        size_t namePos = headersStr.find("name=\"");
        if (namePos != std::string::npos) {
            size_t nameEnd = headersStr.find('\"', namePos + 6);
            if (nameEnd != std::string::npos) {
                name = headersStr.substr(namePos + 6, nameEnd - namePos - 6);
            }
        }

        if (!name.empty()) {
            bodyParams[name] = content;
        }
    }

    /**
     * 自动检测Content-Type并解析
     */
    void autoDetectContentType() {
        if (body.empty()) {
            return;
        }

        // 尝试解析为JSON
        try {
            // 先检查是否是JSON格式
            std::string trimmed = body;
            trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace),
                          trimmed.end());

            if (trimmed.front() == '{' && trimmed.back() == '}') {
                // 尝试解析为JSON
                JsonValue testJson;
                testJson.fromJson(body);
                parseJsonBody();
                return;
            } else if (trimmed.front() == '[' && trimmed.back() == ']') {
                // 数组形式的JSON
                jsonBody = std::make_unique<JsonValue>();
                jsonBody->fromJson(body);
                bodyParams["_json_array"] = body;
                return;
            }
        } catch (const std::exception&) {
            // 不是有效的JSON，继续尝试其他格式
        }

        // 检查是否是表单数据
        if (body.find('=') != std::string::npos &&
            (body.find('&') != std::string::npos || body.find('\n') != std::string::npos)) {
            parseUrlEncodedBody();
            return;
        }

        // 默认作为纯文本
        bodyParams["_raw_text"] = body;
    }

    /**
     * URL解码（将%XX转换为字符）
     */
    static std::string urlDecode(const std::string& str) {
        std::string result;
        result.reserve(str.length());

        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                // 尝试解码%XX格式
                std::string hex = str.substr(i + 1, 2);
                char ch = static_cast<char>(std::strtoul(hex.c_str(), nullptr, 16));
                result.push_back(ch);
                i += 2;
            } else if (str[i] == '+') {
                // +号表示空格
                result.push_back(' ');
            } else {
                result.push_back(str[i]);
            }
        }

        return result;
    }

    /**
     * 不区分大小写的字符串比较辅助函数
     */
    static int strcasecmp(const char* s1, const char* s2) {
        while (*s1 && *s2 && ::tolower(*s1) == ::tolower(*s2)) {
            ++s1;
            ++s2;
        }
        return ::tolower(*s1) - ::tolower(*s2);
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

    void success(const std::map<std::string, std::string>& resMap) {
        std::map<std::string, std::string> result=resMap;
        result["status"] = "ok";
        result["message"] = "Success";
        json(mpToJson(result));
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
    explicit Server(int port = 8080, bool printParams = false);
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
    bool LogParams;
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

    static std::string getLanIpv4();

    void listenIPv6(int serverSocketIPv6);

    static std::string getLanIpv6();
};

#endif // SERVER_H

