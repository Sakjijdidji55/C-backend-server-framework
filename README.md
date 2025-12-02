# C++ 后端服务器框架

一个基于现代C++ (C++17)开发的轻量级、高性能HTTP服务器框架，专为快速构建可靠、可扩展的后端服务而设计。

## 主要特性

- **轻量级HTTP服务器**：基于原生套接字实现，零外部依赖
- **完整的路由系统**：支持GET、POST、PUT、DELETE等HTTP方法的路由注册和处理
- **高效的线程池**：提供并发任务处理机制，优化服务器性能
- **强大的JSON处理**：内置JSON解析和生成功能，支持复杂的嵌套数据结构
- **JWT身份认证**：提供完整的JWT令牌生成和验证功能
- **日志系统**：采用单例模式设计的日志记录功能
- **跨平台兼容**：同时支持Windows和Linux平台
- **现代化C++实践**：充分利用C++17特性，代码清晰易懂

## 系统要求

- C++17或更高版本的编译器
- CMake 3.15或更高版本
- **Windows平台**：Visual Studio 2019+ 或 MinGW
- **Linux平台**：GCC 8+ 或 Clang 7+

## 安装和构建

### 克隆仓库

```bash
git clone <仓库地址>
cd C-backend-server-framework
```

### 使用CMake构建

#### Windows (Visual Studio)

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Linux (GCC/Clang)

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

构建完成后，可执行文件将位于`build`目录下。

## 快速开始

以下是创建基本服务器的简单示例：

```cpp
#include "include/Server.h"
#include "include/route.h"

int main() {
    // 创建服务器实例，监听8080端口
    Server app(8080);
    
    // 注册路由
    app.get("/", [](const Request& req, Response& res) {
        res.json(R"({"message":"Welcome to C++ Server"})";
    });
    
    // 启动服务器
    app.run();
    
    return 0;
}
```

## API 文档

### Server类

`Server`是框架的核心类，负责创建和管理HTTP服务器实例。

#### 构造函数

```cpp
Server(int port = 8080, bool printParams = false)
```
- **参数**:
  - `port` - 服务器监听的端口号，默认为8080
  - `printParams` - 是否打印请求参数，默认为false

#### 路由注册方法

```cpp
void get(const std::string& path, Handler handler);
void post(const std::string& path, Handler handler);
void put(const std::string& path, Handler handler);
void del(const std::string& path, Handler handler);
```
- **参数**:
  - `path` - 路由路径
  - `handler` - 处理请求的回调函数，类型为`std::function<void(const Request&, Response&)>`

#### 服务器控制方法

```cpp
void run();        // 启动服务器，开始监听请求
void stop();       // 停止服务器
static Server* getInstance();  // 获取服务器实例（单例模式）
```

### Request类

`Request`结构体表示客户端的HTTP请求，提供对请求数据的访问。

#### 主要成员变量

```cpp
std::string method;                   // HTTP方法 (GET, POST等)
std::string path;                     // 请求路径
std::map<std::string, std::string> headers;      // 请求头
std::string body;                     // 请求体
std::map<std::string, std::string> queryParams;  // URL查询参数
std::map<std::string, JsonValue> bodyParams;     // 解析后的请求体参数
std::unique_ptr<JsonValue> jsonBody;  // 解析后的JSON请求体
```

#### 主要成员函数

```cpp
std::string query_param(const std::string& key) const;  // 获取指定查询参数值
std::string json_param(const std::string& key) const;   // 获取JSON参数值
void parseBody();           // 根据Content-Type自动解析请求体
bool isJson() const;        // 检查请求体是否为JSON格式
const JsonValue* getJsonBody() const;  // 获取解析后的JSON对象
void show() const;          // 打印请求详情（调试用）
```

### Response类

`Response`结构体用于构建和发送HTTP响应。

#### 主要成员变量

```cpp
int statusCode = 200;                   // HTTP状态码
std::map<std::string, std::string> headers;  // 响应头
std::string body;                       // 响应体
```

#### 主要成员函数

```cpp
void json(const std::string& jsonStr);  // 设置JSON格式响应
void text(const std::string& textStr);  // 设置纯文本响应
void status(int code);                  // 设置HTTP状态码
void success();                         // 设置成功响应
void success(const std::map<std::string, std::string>& resMap);  // 设置带数据的成功响应
void success(const std::map<std::string, JsonValue>& resMap);    // 设置带嵌套JSON的成功响应
void error(int code, const std::string& message);  // 设置错误响应
```

### JsonValue类

`JsonValue`类提供完整的JSON数据类型表示和处理功能。

#### 支持的数据类型

- NULL_TYPE: null值
- STRING: 字符串值
- NUMBER: 数字值（整数或浮点数）
- BOOLEAN: 布尔值
- OBJECT: 对象（键值对映射）
- ARRAY: 数组

#### 主要方法

```cpp
// 构造函数
explicit JsonValue(const std::string& value);
explicit JsonValue(const char* value);
explicit JsonValue(int value);
explicit JsonValue(double value);
explicit JsonValue(bool value);
explicit JsonValue(const std::map<std::string, JsonValue>& value);
explicit JsonValue(const std::vector<JsonValue>& value);

// 类型检查和转换
Type getType() const;
std::string asString() const;
double asNumber() const;
bool asBoolean() const;
std::map<std::string, JsonValue> asObject() const;
std::vector<JsonValue> asArray() const;

// JSON转换
std::string toJson() const;  // 转换为JSON字符串
void fromJson(const std::string& jsonStr);  // 从JSON字符串解析

// 静态辅助方法
static JsonValue object(const std::map<std::string, JsonValue>& obj);
static JsonValue array(const std::vector<JsonValue>& arr);
```

### ThreadPool类

`ThreadPool`类提供线程池实现，用于并发处理客户端请求。

#### 构造函数

```cpp
ThreadPool(size_t numThreads)
```
- **参数**: `numThreads` - 线程池中的线程数量

#### 添加任务方法

```cpp
template<typename F, typename... Args>
bool addTask(F&& f, Args&&... args);
```
- **参数**:
  - `f` - 要执行的函数
  - `args` - 函数参数
- **返回值**: 任务添加成功返回true，队列已满返回false

### Log类

`Log`类提供日志记录功能，采用单例模式设计。

#### 主要方法

```cpp
// 获取单例实例
static Log* getInstance();

// 写入日志消息
void write(const std::string& msg);
```

## 项目结构

```
├── .github/             # GitHub配置目录
│   └── workflows/       # GitHub Actions工作流
├── .gitignore           # Git忽略配置
├── CMakeLists.txt       # CMake构建配置
├── LICENSE              # 许可证文件
├── README.md            # 中文项目说明文档
├── README.en.md         # 英文项目说明文档
├── include/             # 头文件目录
│   ├── Handler.h        # 请求处理器头文件
│   ├── JsonValue.h      # JSON处理头文件
│   ├── Log.h            # 日志系统头文件
│   ├── Server.h         # 服务器核心头文件
│   ├── Threadpool.h     # 线程池头文件
│   ├── jwt.h            # JWT认证头文件
│   └── route.h          # 路由系统头文件
├── main.cpp             # 主程序入口
└── src/                 # 源代码目录
    ├── JsonValue.cpp    # JSON处理实现
    ├── Log.cpp          # 日志系统实现
    ├── Server.cpp       # 服务器核心实现
    ├── Threadpool.cpp   # 线程池实现
    ├── jwt.cpp          # JWT认证实现
    └── route.cpp        # 路由系统实现
```

## 路由示例

### 基本路由

```cpp
// GET路由
app.get("/api/users", [](const Request& req, Response& res) {
    res.json(R"({"users": ["user1", "user2"]})";
});

// POST路由
app.post("/api/users", [](const Request& req, Response& res) {
    // 处理用户创建逻辑
    res.status(201);  // Created
    res.json(R"({"status": "success", "message": "用户创建成功"})";
});
```

### 获取查询参数

```cpp
app.get("/api/search", [](const Request& req, Response& res) {
    std::string query = req.query_param("q");
    res.json(R"({"query": "")" + query + R"(", "results": []})";
});
```

### 处理JSON请求

```cpp
app.post("/api/data", [](const Request& req, Response& res) {
    if (req.isJson()) {
        const JsonValue* json = req.getJsonBody();
        std::string name = req.json_param("name");
        res.json(R"({"status": "success", "received": "")" + name + R"("})";
    } else {
        res.error(400, "无效的JSON数据");
    }
});
```

### 使用JWT进行身份验证

```cpp
// 创建JWT实例
JWT jwt("your-secret-key", 3600);  // 密钥和过期时间（秒）

// 登录路由
app.post("/api/login", [&jwt](const Request& req, Response& res) {
    // 验证用户凭据
    std::string username = req.json_param("username");
    std::string password = req.json_param("password");
    
    // 实际应用中需验证用户名和密码
    if (!username.empty() && !password.empty()) {
        // 生成JWT令牌
        std::map<std::string, std::string> claims = {{"username", username}};
        std::string token = jwt.generateToken(claims);
        
        res.json(R"({"status": "success", "token": "")" + token + R"("})";
    } else {
        res.error(401, "用户名或密码错误");
    }
});

// 受保护的路由
app.get("/api/protected", [&jwt](const Request& req, Response& res) {
    // 从请求头获取令牌
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) {
        res.error(401, "缺少Authorization头");
        return;
    }
    
    std::string authHeader = it->second;
    // 假设格式为 "Bearer <token>"
    size_t bearerPos = authHeader.find("Bearer ");
    if (bearerPos == std::string::npos || authHeader.substr(bearerPos + 7).empty()) {
        res.error(401, "无效的Authorization头格式");
        return;
    }
    
    std::string token = authHeader.substr(bearerPos + 7);
    
    // 验证令牌
    if (jwt.verifyToken(token)) {
        res.json(R"({"status": "success", "data": "访问受保护资源成功"})";
    } else {
        res.error(401, "无效或已过期的令牌");
    }
});
```

## 跨平台注意事项

- **Windows平台**：内部使用Winsock2库，CMake会自动处理依赖
- **Linux平台**：需要链接`pthread`库，CMake会自动处理
- 信号处理机制在不同平台有所不同，但框架已封装好，用户无需关心具体实现细节

## 许可证

[MIT License](LICENSE)

## 贡献指南

欢迎提交问题报告和改进建议！如有任何问题或想参与项目开发，请随时联系我们。

## 版本历史

- 初始版本：提供基本的HTTP服务器功能、路由系统和JSON处理
- 最新版本：增加了JWT认证、改进了日志系统、优化了JSON嵌套数据处理
