/**
 * @file Server.h
 * @brief 服务器核心类头文件
 * @brief Server core class header file
 */
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
#include <winsock2.h>    // Windows平台网络头文件 (Windows platform network header)
#include <windows.h>
#include <fstream>

#else
#include <sys/socket.h>  // Linux/Unix平台网络头文件 (Linux/Unix platform network header)
#include <netinet/in.h>  // 包含sockaddr_in定义 (Contains sockaddr_in definition)
#include <arpa/inet.h>
#endif

#include "Threadpool.h"

#include "Log.h"

std::string mpToJson(const std::map<std::string, std::string>& mp);

std::string getFormattedDate();

#include "JsonValue.h"

/**
 * @struct Request
 * @brief HTTP请求结构体
 * @brief HTTP request structure
 */
struct Request {
    std::string method;                      ///< HTTP方法 (HTTP method)
    std::string path;                        ///< 请求路径 (Request path)
    std::map<std::string, std::string> headers; ///< HTTP头部信息 (HTTP headers)
    std::string body;                        ///< 请求体 (Request body)
    std::map<std::string, std::string> queryParams;  ///< 查询参数 (Query parameters)
    std::map<std::string, JsonValue> bodyParams;   ///< 表单参数或JSON参数 (Form or JSON parameters)
    std::unique_ptr<JsonValue> jsonBody;            ///< 解析后的JSON对象（如果适用）(Parsed JSON object if applicable)

    /**
     * @brief 获取查询参数的值
     * @param key 查询参数名
     * @return 查询参数值，如果不存在则返回空字符串
     * @brief Get the value of a query parameter
     * @param key Query parameter name
     * @return Query parameter value, or empty string if not exists
     */
    [[nodiscard]] std::string query_param(const std::string& key) const {
        auto it = queryParams.find(key);
        return it != queryParams.end() ? it->second : "";
    }

    /**
     * @brief 显示请求的详细信息
     * @brief Display detailed request information
     */
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
                std::cout << "  " << key << " = " << value.toJson() << "\n";
            }
        }
    }

    /**
     * @brief 解析请求体，根据Content-Type处理不同格式
     * @brief Parse request body, handle different formats based on Content-Type
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
            bodyParams["_raw_text"] = JsonValue(body);
        } else if (contentType.empty()) {
            // 没有Content-Type，尝试自动检测
            autoDetectContentType();
        } else {
            // 未知类型，作为原始文本处理
            std::cerr << "Warning: Unknown Content-Type: " << contentType << "\n";
            bodyParams["_raw_data"] = JsonValue(body);
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

                std::cout<<value.size()<<std::endl;

                bodyParams[key] = JsonValue(value);
            } else {
                // 只有key没有value的情况
                bodyParams[urlDecode(pair)] = JsonValue("");
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
                bodyParams["_json_array"] = *jsonBody;
            } else {
                // 其他基本类型
                bodyParams["_json_value"] = *jsonBody;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON body: " << e.what() << "\n";
            // JSON解析失败，尝试作为普通文本处理
            bodyParams["_invalid_json"] = JsonValue(body);
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
                    bodyParams[newKey] = value;
                } else {
                    // 基本类型转换为字符串
                    bodyParams[newKey] = value;
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
            bodyParams[name] = JsonValue(content);
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
                bodyParams["_json_array"] = JsonValue(body);
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
        bodyParams["_raw_text"] = JsonValue(body);
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

/**
 * @struct Response
 * @brief HTTP响应结构体
 * @brief HTTP response structure
 */
struct Response {
    int statusCode = 200;                          ///< HTTP状态码 (HTTP status code)
    std::map<std::string, std::string> headers; ///< HTTP响应头 (HTTP response headers)
    std::string body;                              ///< 响应体 (Response body)
    
    /**
     * @brief 构造函数，初始化默认响应头
     * @brief Constructor, initializes default response headers
     */
    Response() {
        headers["Content-Type"] = "application/json; charset=utf-8";
    }
    
    /**
     * @brief 设置JSON响应
     * @param jsonStr JSON格式的响应内容
     * @brief Set JSON response
     * @param jsonStr JSON formatted response content
     */
    void json(const std::string& jsonStr) {
        body = jsonStr;
        headers["Content-Type"] = "application/json; charset=utf-8";
        Log::getInstance()->write("Time "+getFormattedDate()+ " Code: " + std::to_string(statusCode) +" Response: " + jsonStr + "\n");
    }
    
    /**
     * @brief 设置文本响应
     * @param textStr 文本格式的响应内容
     * @brief Set text response
     * @param textStr Text formatted response content
     */
    void text(const std::string& textStr) {
        body = textStr;
        headers["Content-Type"] = "text/plain; charset=utf-8";
        Log::getInstance()->write("Time "+getFormattedDate()+ " Code: " + std::to_string(statusCode) +" Response: " + textStr + "\n");
    }

    /**
     * @brief 设置HTTP状态码
     * @param code HTTP状态码
     * @brief Set HTTP status code
     * @param code HTTP status code
     */
    void status(int code) {
        statusCode = code;
    }

    /**
     * @brief 设置成功响应（带状态和消息）
     * @param resMap 要返回的数据映射
     * @brief Set success response (with status and message)
     * @param resMap Data map to return
     */
    void success(const std::map<std::string, std::string>& resMap) {
        std::map<std::string, std::string> result=resMap;
        result["status"] = "ok";
        result["message"] = "Success";
        json(mpToJson(result));
        Log::getInstance()->write("Time "+getFormattedDate()+ " Code: " + std::to_string(statusCode) +" Response: " + mpToJson(result) + "\n");
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
        Log::getInstance()->write("Time "+getFormattedDate()+ " Code: " + std::to_string(statusCode) +" Response: " + toJson(result) + "\n");
    }

    void success() {
        json(R"({"status":"ok", "message":"Success"})");
        Log::getInstance()->write("Time "+getFormattedDate()+ " Code: " + std::to_string(statusCode) +" Response: " + R"({"status":"ok", "message":"Success"})" + "\n");
    }

    void error(int code, const std::string& message) {
        statusCode = code;
        json(R"({"status":"fail", "message":")" + message + "\"}");
        Log::getInstance()->write("Time "+getFormattedDate()+" Code "+std::to_string(code)+" Error: " + message + "\n");
    }

    /**
     * 传输二进制文件数据（优化版：自动处理文件名和后缀）
     * @param filePath 本地文件路径
     * @param mimeType 文件MIME类型（如image/png、application/octet-stream）
     * @param isAttachment 是否作为附件下载（true=触发下载，false=浏览器内展示）
     * @param customFileName 自定义下载文件名（为空则使用原文件名称）
     */
    void file(const std::string& filePath,
              const std::string& mimeType = "application/octet-stream",
              bool isAttachment = true,
              const std::string& customFileName = "") {
        try {
            // 1. 以二进制模式打开文件
            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                error(404, "File not found: " + filePath); // 修正：文件不存在应返回404而非400
                return;
            }

            // 2. 获取文件大小并分配缓冲区
            std::streamsize fileSize = file.tellg();
            if (fileSize <= 0) {
                file.close();
                error(400, "File is empty: " + filePath);
                return;
            }
            file.seekg(0, std::ios::beg);

            // 3. 读取二进制数据到body
            body.resize(fileSize);
            if (!file.read(&body[0], fileSize)) {
                file.close();
                error(500, "Failed to read file: " + filePath);
                return;
            }
            file.close();

            // 4. 核心：处理文件名和后缀，设置响应头
            statusCode = 200;
            headers["Content-Type"] = mimeType;                  // 设置MIME类型
            headers["Content-Length"] = std::to_string(fileSize); // 设置数据长度
            headers["Content-Transfer-Encoding"] = "binary";     // 声明二进制传输

            // 关键：获取文件名（包含后缀），告诉浏览器文件类型
            std::string fileName = customFileName.empty() ? getFileNameWithExt(filePath) : customFileName;

            // 设置Content-Disposition，明确告诉浏览器文件名和后缀
            if (isAttachment) {
                // 触发下载（浏览器会保存文件，文件名带后缀）
                headers["Content-Disposition"] = "attachment; filename=\"" + fileName + "\"";
            } else {
                // 浏览器内展示（如图片直接显示，仍保留文件名标识）
                headers["Content-Disposition"] = "inline; filename=\"" + fileName + "\"";
            }

            Log::getInstance()->write("Time "+getFormattedDate()+ " Code: " + std::to_string(statusCode) +" Response: " + fileName + "\n");

        } catch (const std::exception& e) {
            error(500, "File transfer error: " + std::string(e.what()));
        }
    }

    // 辅助方法：从文件路径中提取带后缀的完整文件名
    static std::string getFileNameWithExt(const std::string& filePath) {
        // 处理 Windows(\) 和 Linux/Mac(/) 路径分隔符
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string fileName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);

        // 确保文件名非空（防止路径以分隔符结尾）
        if (fileName.empty()) {
            fileName = "unknown_file";
        }
        return fileName;
    }
};

/**
 * @typedef Handler
 * @brief 路由处理器函数类型
 * @brief Route handler function type
 */
typedef std::function<void(const Request&, Response&)> Handler;

/**
 * @class Server
 * @brief 简单的HTTP服务器类
 * @brief Simple HTTP server class
 */
class Server {
public:
    /**
     * @brief 构造函数，初始化服务器端口
     * @param port 服务器监听端口，默认8080
     * @param printParams 是否打印请求参数
     * @brief Constructor, initialize server port
     * @param port Server listening port, default 8080
     * @param printParams Whether to print request parameters
     */
    explicit Server(int port = 8080, bool printParams = false);
    
    /**
     * @brief 析构函数，清理资源
     * @brief Destructor, clean up resources
     */
    ~Server();
    
    /**
     * @brief 注册GET方法路由
     * @param path 路由路径
     * @param handler 处理函数
     * @brief Register GET method route
     * @param path Route path
     * @param handler Handler function
     */
    void get(const std::string& path, Handler handler);
    
    /**
     * @brief 注册POST方法路由
     * @param path 路由路径
     * @param handler 处理函数
     * @brief Register POST method route
     * @param path Route path
     * @param handler Handler function
     */
    void post(const std::string& path, Handler handler);
    
    /**
     * @brief 注册PUT方法路由
     * @param path 路由路径
     * @param handler 处理函数
     * @brief Register PUT method route
     * @param path Route path
     * @param handler Handler function
     */
    void put(const std::string& path, Handler handler);
    
    /**
     * @brief 注册DELETE方法路由
     * @param path 路由路径
     * @param handler 处理函数
     * @brief Register DELETE method route
     * @param path Route path
     * @param handler Handler function
     */
    void del(const std::string& path, Handler handler);
    
    /**
     * @brief 启动服务器
     * @brief Start server
     */
    void run();
    
    /**
     * @brief 停止服务器
     * @brief Stop server
     */
    void stop();

    /**
     * @brief 获取服务器实例（单例模式）
     * @return Server* 服务器实例指针
     * @brief Get server instance (singleton pattern)
     * @return Server* Server instance pointer
     */
    static Server* getInstance();
private:
    int port_;                            ///< 服务器端口 (Server port)
    int serverSocket_;                    ///< 服务器套接字 (Server socket)
    bool running_;                        ///< 服务器运行状态 (Server running status)
    bool LogParams;                       ///< 是否记录参数 (Whether to log parameters)
    std::mutex routesMutex_;              ///< 路由表互斥锁 (Routes mutex lock)
    std::mutex logMutex_;                 ///< 日志互斥锁 (Log mutex lock)
    ThreadPool threadpool_;               ///< 线程池 (Thread pool)
    
    /**
     * @brief 路由表：method -> (path -> handler)
     * @brief Routes table: method -> (path -> handler)
     */
    std::map<std::string, std::map<std::string, Handler>> routes_;
    
    /**
     * @brief 静态成员用于信号处理
     * @brief Static member for signal handling
     */
    static Server* instance_;
    
    /**
     * @brief 静态信号处理函数
     * @param sig 信号值
     * @brief Static signal handler function
     * @param sig Signal value
     */
    static void signalHandler(int sig);
    
    /**
     * @brief 注册信号处理器
     * @brief Register signal handler
     */
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

