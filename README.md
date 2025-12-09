# C++ HTTP 服务器
# C++ HTTP Server

## 项目概述
## Project Overview

这是一个用C++实现的轻量级HTTP服务器，提供了完整的Web服务功能，包括路由处理、数据库连接、JSON解析、JWT认证、线程池和日志记录等特性。

This is a lightweight HTTP server implemented in C++, providing complete web service functionality including routing, database connection, JSON parsing, JWT authentication, thread pool, and logging features.

## 功能特性
## Features

- **HTTP服务器核心**：支持HTTP/1.1协议，处理GET、POST、PUT、DELETE等请求方法
- **数据库连接**：基于MySQL的数据库访问接口，实现单例模式
- **JSON处理**：支持JSON解析和序列化
- **JWT认证**：实现基于HS256算法的JWT令牌生成和验证
- **线程池**：多线程处理请求，提高并发性能
- **日志系统**：文件日志记录，支持单例模式
- **跨平台支持**：兼容Windows和Linux系统

- **HTTP Server Core**: Supports HTTP/1.1 protocol, handling GET, POST, PUT, DELETE and other request methods
- **Database Connection**: MySQL-based database access interface, implemented with singleton pattern
- **JSON Processing**: Supports JSON parsing and serialization
- **JWT Authentication**: Implements JWT token generation and verification based on HS256 algorithm
- **Thread Pool**: Multi-threaded request handling for improved concurrent performance
- **Logging System**: File-based logging with singleton pattern
- **Cross-platform Support**: Compatible with Windows and Linux systems

## 系统要求
## System Requirements

- C++17 或更高版本
- CMake 3.10 或更高版本
- MySQL 客户端库
- Windows 或 Linux 操作系统

- C++17 or higher
- CMake 3.10 or higher
- MySQL client library
- Windows or Linux operating system

## 目录结构
## Directory Structure

```
c++Server/
├── .github/          # GitHub 配置文件
├── 3rdparty/         # 第三方库
├── include/          # 头文件
├── model/            # 数据模型
├── src/              # 源代码文件
│   ├── Server.cpp    # HTTP服务器实现
│   ├── DBConnector.cpp # 数据库连接器
│   ├── JsonValue.cpp  # JSON处理
│   ├── Log.cpp       # 日志系统
│   ├── Threadpool.cpp # 线程池实现
│   ├── jwt.cpp       # JWT认证实现
│   └── main.cpp      # 主程序入口
└── README.md         # 项目文档
```

## 安装说明
## Installation

### 编译项目
### Building the Project

1. 克隆仓库
1. Clone the repository

```bash
git clone <repository-url>
cd c++Server
```

2. 创建构建目录
2. Create build directory

```bash
mkdir build
cd build
```

3. 配置CMake
3. Configure CMake

```bash
cmake ..
```

4. 编译项目
4. Build the project

```bash
make  # Linux
build  # Windows
```

## 使用方法
## Usage

### 运行服务器
### Running the Server

```bash
./c++Server  # Linux
c++Server.exe  # Windows
```

### 默认配置
### Default Configuration

- 默认端口：8080
- 默认数据库地址：localhost:3306
- 默认日志文件：server.log

- Default port: 8080
- Default database address: localhost:3306
- Default log file: server.log

## API 示例
## API Examples

### 发送请求
### Sending Requests

```bash
# GET 请求示例
curl http://localhost:8080/api/data

# POST 请求示例
curl -X POST http://localhost:8080/api/login -H "Content-Type: application/json" -d '{"username":"admin","password":"123456"}'
```

### JWT认证
### JWT Authentication

1. 获取令牌
1. Get token

```bash
curl -X POST http://localhost:8080/api/auth -H "Content-Type: application/json" -d '{"username":"user","password":"pass"}'
```

2. 使用令牌访问受保护资源
2. Access protected resources using token

```bash
curl http://localhost:8080/api/protected -H "Authorization: Bearer <your-token>"
```

## 代码注释
## Code Comments

项目中的所有C++源代码都包含详细的中英双语注释，包括：

- 文件头注释：说明文件功能和主要组件
- 类注释：描述类的用途和设计模式
- 方法注释：包含参数说明、返回值和异常信息
- 行内注释：解释关键代码逻辑

All C++ source code in the project contains detailed bilingual Chinese-English comments, including:

- File header comments: explaining file functionality and main components
- Class comments: describing class purpose and design patterns
- Method comments: including parameter descriptions, return values, and exception information
- Inline comments: explaining key code logic

## 许可证
## License

[MIT License](LICENSE)

## 贡献指南
## Contributing

欢迎提交Issue和Pull Request来改进这个项目！

Contributions are welcome! Please feel free to submit issues and pull requests to improve this project.

## 联系方式
## Contact

如有任何问题或建议，请联系项目维护者。

For any questions or suggestions, please contact the project maintainers.
