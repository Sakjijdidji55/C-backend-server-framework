# C++ 后端服务器框架

一个用现代C++ (C++17/20)编写的轻量级、高性能后端服务器框架，专为构建可扩展、可靠且易于维护的后端服务而设计。

## 特性

- **轻量级HTTP服务器**：基于原生套接字实现，无外部依赖
- **路由系统**：支持GET、POST、PUT、DELETE等HTTP方法的路由注册
- **线程池**：高效的并发处理机制，提高服务器性能
- **JSON支持**：内置JSON解析和生成功能，支持嵌套数据结构
- **JWT认证**：提供完整的JWT令牌生成和验证功能
- **日志系统**：简单易用的日志记录功能
- **跨平台**：同时支持Windows和Linux平台
- **现代C++**：充分利用C++17/20特性，代码清晰易读

## 系统要求

- C++17或更高版本的编译器
- CMake 3.15或更高版本
- Windows: Visual Studio 2019+ 或 MinGW
- Linux: GCC 8+ 或 Clang 7+

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

构建完成后，可执行文件将位于`build/bin`目录下。

## 快速开始

以下是一个简单的使用示例，展示如何创建一个基本的服务器：

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

`Server`是框架的核心类，负责创建和管理HTTP服务器。

#### 构造函数

```cpp
Server(int port = 8080)
```
- **参数**: `port` - 服务器监听的端口号，默认为8080

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
void run();        // 启动服务器
void stop();       // 停止服务器
```

### Request类

`Request`表示客户端的HTTP请求。

#### 成员变量

```cpp
std::string method;           // HTTP方法 (GET, POST等)
std::string path;             // 请求路径
std::map<std::string, std::string> headers;  // 请求头
std::string body;             // 请求体
std::map<std::string, std::string> queryParams;  // 查询参数
std::map<std::string, std::string> bodyParams;   // 表单参数
```

#### 成员函数

```cpp
std::string query_param(const std::string& key) const;  // 获取查询参数
void parseBody();          // 解析请求体
void show() const;         // 显示请求详情
```

### Response类

`Response`用于构建服务器的HTTP响应。

#### 成员变量

```cpp
int statusCode = 200;       // HTTP状态码
std::map<std::string, std::string> headers;  // 响应头
std::string body;           // 响应体
```

#### 成员函数

```cpp
void json(const std::string& jsonStr);  // 设置JSON响应
void text(const std::string& textStr);  // 设置文本响应
void status(int code);       // 设置状态码
void success();              // 设置成功响应
void success(const std::map<std::string, std::string>& resMap);  // 设置带数据的成功响应
void success(const std::map<std::string, JsonValue>& resMap);    // 设置嵌套JSON的成功响应
void error(int code, const std::string& message);  // 设置错误响应
```

### JsonValue类

`JsonValue`提供完整的JSON值表示，支持嵌套的JSON数据结构。

#### 支持的类型

- NULL_TYPE: null值
- STRING: 字符串值
- NUMBER: 数字值
- BOOLEAN: 布尔值
- OBJECT: 对象（键值对映射）
- ARRAY: 数组

#### 主要方法

```cpp
// 构造函数
explicit JsonValue(const std::string& value);
explicit JsonValue(int value);
explicit JsonValue(double value);
explicit JsonValue(bool value);
JsonValue(const std::map<std::string, JsonValue>& value);
JsonValue(const std::vector<JsonValue>& value);

// 获取类型
Type getType() const;
```

### JWT类

`JWT`提供JWT令牌的生成和验证功能。

#### 主要方法

```cpp
// 设置密钥
void setSecret(const std::string& secret);

// 生成令牌
std::string generateToken(const std::map<std::string, std::string>& customClaims = {}, 
                          long long ttlSeconds = -1) const;

// 验证令牌
bool verifyToken(const std::string& token, std::string* payloadJson = nullptr) const;

// 解析声明
std::optional<std::map<std::string, std::string>> parseClaims(const std::string& token) const;

// 密码加密和验证
static std::string encrypt_password(const std::string &password);
static bool verify_password(const std::string& password, const std::string& stored_hash);
```

### ThreadPool类

`ThreadPool`提供线程池实现，用于并发处理客户端请求。

#### 构造函数

```cpp
explicit ThreadPool(size_t numThreads);
```
- **参数**: `numThreads` - 线程池中的线程数量

#### 添加任务

```cpp
template<typename F, typename... Args>
bool addTask(F&& f, Args&&... args);
```
- **参数**:
  - `f` - 要执行的函数
  - `args` - 函数参数
- **返回值**: 任务添加成功返回true，队列已满返回false

### Log类

`Log`提供日志记录功能。

#### 主要方法

```cpp
// 获取单例实例
static Log* getInstance();

// 写入日志
void write(const std::string& msg);
```

## 项目结构

```
├── .gitignore          # Git忽略文件
├── CMakeLists.txt      # CMake构建配置
├── LICENSE             # 许可证文件
├── README.md           # 项目说明文档
├── include/            # 头文件目录
│   ├── Headler.h       # 请求处理器头文件
│   ├── JsonValue.h     # JSON处理头文件
│   ├── Log.h           # 日志系统头文件
│   ├── Server.h        # 服务器核心头文件
│   ├── Threadpool.h    # 线程池头文件
│   ├── jwt.h           # JWT认证头文件
│   └── route.h         # 路由系统头文件
├── main.cpp            # 主程序入口
└── src/                # 源代码目录
    ├── JsonValue.cpp   # JSON处理实现
    ├── Log.cpp         # 日志系统实现
    ├── Server.cpp      # 服务器核心实现
    ├── Threadpool.cpp  # 线程池实现
    ├── jwt.cpp         # JWT认证实现
    └── route.cpp       # 路由系统实现
```

## 路由示例

以下是一些常见的路由注册示例：

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
    res.json(R"({"status": "success", "message": "User created"})";
});
```

### 获取查询参数

```cpp
app.get("/api/search", [](const Request& req, Response& res) {
    std::string query = req.query_param("q");
    res.json(R"({"query": "" + query + "", "results": []})";
});
```

### 使用JWT进行身份验证

```cpp
// 创建JWT实例
JWT jwt("your-secret-key", 3600);  // 密钥和过期时间（秒）

// 登录路由
app.post("/api/login", [&jwt](const Request& req, Response& res) {
    // 验证用户凭据（示例中简化处理）
    bool validCredentials = true;  // 实际应用中需验证用户名和密码
    
    if (validCredentials) {
        // 生成JWT令牌
        std::map<std::string, std::string> claims = {{"username", "user123"}};
        std::string token = jwt.generateToken(claims);
        
        res.json(R"({"status": "success", "token": "" + token + ""})";
    } else {
        res.error(401, "Invalid credentials");
    }
});

// 受保护的路由（需要验证令牌）
app.get("/api/protected", [&jwt](const Request& req, Response& res) {
    // 从请求头获取令牌
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) {
        res.error(401, "Authorization header required");
        return;
    }
    
    std::string authHeader = it->second;
    // 假设格式为 "Bearer <token>"
    size_t bearerPos = authHeader.find("Bearer ");
    if (bearerPos == std::string::npos || authHeader.substr(bearerPos + 7).empty()) {
        res.error(401, "Invalid authorization header format");
        return;
    }
    
    std::string token = authHeader.substr(bearerPos + 7);
    
    // 验证令牌
    if (jwt.verifyToken(token)) {
        res.json(R"({"status": "success", "data": "Protected resource accessed"})";
    } else {
        res.error(401, "Invalid or expired token");
    }
});
```

## 跨平台注意事项

- **Windows**: 需要链接`ws2_32`库，CMake会自动处理
- **Linux**: 需要链接`pthread`库，CMake会自动处理
- 信号处理机制在不同平台有所不同，但框架已做了封装，用户无需关心

## 许可证

[MIT License](LICENSE)

## 贡献

欢迎提交问题和拉取请求。如果您有任何建议或改进，请随时联系我们。
