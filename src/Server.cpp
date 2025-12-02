#include "../include/Server.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <cstring>
#include <utility>
#include <signal.h>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif
    #define close(s) closesocket(s)
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>     // 包含 hostent、gethostbyname、addrinfo、getaddrinfo（Linux/macOS）
    #include <unistd.h>    // 包含 close（Linux/macOS）
#endif


/**
 * 将map转换为JSON格式的字符串
 * Convert map to JSON formatted string
 * @param mp 输入的map，键值类型均为string
 * Input map with string key and value types
 * @return 返回JSON格式的字符串
 * Returns JSON formatted string
 */
std::string mpToJson(const std::map<std::string, std::string>& mp) {
    std::string json = "{";  // 初始化JSON字符串，开始标记
    // 遍历map中的每个键值对
    for (const auto& pair : mp) {
        // 将键值对添加到JSON字符串中，格式为"key":"value",
        json += "\"" + pair.first + "\":\"" + pair.second + "\",";
    }
    // 如果JSON字符串不为空（即map不为空），移除最后一个多余的逗号
    if (!json.empty()) {
        json.pop_back(); // 移除最后一个逗号
    }
    json += "}";  // 添加JSON字符串的结束标记
    return json;  // 返回生成的JSON字符串
}

// 静态成员初始化
Server* Server::instance_ = nullptr;

Server::Server(int port, bool printParams) : port_(port),serverSocket_(-1), running_(false), LogParams(printParams), threadpool_(std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1) {
    instance_ = this;
    registerSignalHandlers(); // 注册信号处理函数
}

void Server::signalHandler(int /*sig*/) {
    std::cout << "\nShutting down server..." << std::endl;
    if (instance_) {
        instance_->stop();
    }
    exit(0);
}

#ifdef _WIN32
// Windows控制台关闭事件处理函数
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT || 
        dwCtrlType == CTRL_BREAK_EVENT) {
        if (Server::instance_) {
            std::cout << "\nShutting down server..." << std::endl;
            Server::instance_->stop();
        }
        return TRUE;
    }
    return FALSE;
}
#endif

/**
 * 注册信号处理函数，用于处理程序终止信号
 * 根据不同操作系统平台设置相应的信号处理机制
 * Register signal handlers for program termination signals
 * Configure appropriate signal handling mechanisms based on different operating systems
 */
void Server::registerSignalHandlers() {
#ifdef _WIN32
    // Windows上使用SetConsoleCtrlHandler处理控制台关闭事件
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#else
    // Linux/Unix上信号处理正常工作
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#endif
}

Server::~Server() {
    std::cout << "\nShutting down server..." << std::endl;
    stop();
}

void Server::get(const std::string& path, Handler handler) {
    std::lock_guard<std::mutex> lock(routesMutex_);
    routes_["GET"][path] = std::move(handler);
}

void Server::post(const std::string& path, Handler handler) {
    std::lock_guard<std::mutex> lock(routesMutex_);
    routes_["POST"][path] = std::move(handler);
}

void Server::put(const std::string& path, Handler handler) {
    std::lock_guard<std::mutex> lock(routesMutex_);
    routes_["PUT"][path] = std::move(handler);
}

void Server::del(const std::string& path, Handler handler) {
    std::lock_guard<std::mutex> lock(routesMutex_);
    routes_["DELETE"][path] = std::move(handler);
}

/**
 * 服务器运行函数
 * 初始化网络环境，创建socket，绑定地址，监听连接并处理客户端请求
 */
void Server::run() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: Failed to initialize Winsock" << std::endl;
        return;
    }
#endif

    // 1. 创建并绑定IPv4 socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error: Failed to create IPv4 socket" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    // 设置socket选项
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // 绑定IPv4地址
    struct sockaddr_in address{};
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);  // 使用port_成员变量

    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error: Failed to bind IPv4 to port " << port_
                  << ": " << strerror(errno) << std::endl;
#ifdef _WIN32
        closesocket(serverSocket_);
        WSACleanup();
#else
        close(serverSocket_);
#endif
        return;
    }

    // 2. 创建并绑定IPv6 socket
    int serverSocketIPv6 = -1;
    serverSocketIPv6 = socket(AF_INET6, SOCK_STREAM, 0);
    if (serverSocketIPv6 < 0) {
        std::cerr << "Warning: Failed to create IPv6 socket: "
                  << strerror(errno) << std::endl;
        // 不退出，继续使用IPv4
    } else {
        // 设置socket选项
#ifdef _WIN32
        setsockopt(serverSocketIPv6, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(serverSocketIPv6, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        // 在Windows上需要设置IPV6_V6ONLY
#ifdef _WIN32
        int ipv6Only = 1;
        setsockopt(serverSocketIPv6, IPPROTO_IPV6, IPV6_V6ONLY,
                   (const char*)&ipv6Only, sizeof(ipv6Only));
#endif

        // 绑定IPv6地址 - 使用相同的端口号
        struct sockaddr_in6 addr6{};
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port_);  // 使用port_成员变量，确保端口一致

        if (bind(serverSocketIPv6, (struct sockaddr*)&addr6, sizeof(addr6)) < 0) {
            std::cerr << "Warning: Failed to bind IPv6 to port " << port_
                      << ": " << strerror(errno) << std::endl;
#ifdef _WIN32
            closesocket(serverSocketIPv6);
#else
            close(serverSocketIPv6);
#endif
            serverSocketIPv6 = -1;  // 标记为无效
        }
    }

    // 3. 开始监听
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error: Failed to listen on IPv4 socket: "
                  << strerror(errno) << std::endl;
#ifdef _WIN32
        closesocket(serverSocket_);
        if (serverSocketIPv6 >= 0) closesocket(serverSocketIPv6);
        WSACleanup();
#else
        close(serverSocket_);
        if (serverSocketIPv6 >= 0) close(serverSocketIPv6);
#endif
        return;
    }

    // 如果IPv6 socket创建并绑定成功，开始监听
    if (serverSocketIPv6 >= 0) {
        if (listen(serverSocketIPv6, 10) < 0) {
            std::cerr << "Warning: Failed to listen on IPv6 socket: "
                      << strerror(errno) << std::endl;
#ifdef _WIN32
            closesocket(serverSocketIPv6);
#else
            close(serverSocketIPv6);
#endif
            serverSocketIPv6 = -1;
        } else {
            std::cout << "IPv6 socket listening on [::]:" << port_ << std::endl;
            // 启动独立线程监听IPv6连接
            std::thread([this, serverSocketIPv6]() {
                this->listenIPv6(serverSocketIPv6);
            }).detach();
        }
    }

    running_ = true;

    // 打印服务器信息
    printRegisteredRoutes();
    std::cout << "Server running on:" << std::endl;
    std::cout << "  Localhost: http://localhost:" << port_ << std::endl;
    std::cout << "  LAN IPv4:  http://" << getLanIpv4() << ":" << port_ << std::endl;
    std::cout << "  Localhost IPv6: http://[::1]:" << port_ << std::endl;
    if (serverSocketIPv6 >= 0) {
        std::cout << "  LAN IPv6:  http://["<< getLanIpv6() << "]:" << port_ << std::endl;
    }
    std::cout << "==========================================="<< std::endl;

    // 主循环接受IPv4连接
    while (running_) {
        struct sockaddr_in clientAddress{};
        socklen_t clientAddrLen = sizeof(clientAddress);

        int clientSocket = accept(serverSocket_,
                                  (struct sockaddr*)&clientAddress,
                                  &clientAddrLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error: Failed to accept IPv4 connection: "
                          << strerror(errno) << std::endl;
            }
            continue;
        }

        // 添加到线程池
        threadpool_.addTask(&Server::handleClient, this,
                            clientSocket, &clientAddress);
    }
}

void Server::listenIPv6(int serverSocketIPv6) {
    while (running_) {
        struct sockaddr_storage clientAddr{};  // 使用sockaddr_storage通用结构
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocketIPv6,
                                  (struct sockaddr*)&clientAddr,
                                  &clientAddrLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error: Failed to accept IPv6 connection: "
                          << strerror(errno) << std::endl;
            }
            continue;
        }

        // 根据地址族传递正确的参数
        if (clientAddr.ss_family == AF_INET6) {
            threadpool_.addTask(&Server::handleClient, this,
                                clientSocket,
                                (struct sockaddr_in*)&clientAddr);
        }
    }

#ifdef _WIN32
    closesocket(serverSocketIPv6);
#else
    close(serverSocketIPv6);
#endif
}

std::string Server::getLanIpv4() {
    std::string lanIp = "127.0.0.1"; // 默认本地回环（获取失败时返回）
    int sockfd = -1;

#ifdef _WIN32
    // Windows 初始化 Winsock（仅当前函数内临时使用，避免影响全局）
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return lanIp;
    }
#endif

    // 1. 创建 UDP socket（SOCK_DGRAM），仅用于获取本地 IP，不实际发送数据
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return lanIp;
    }

    // 2. 配置公网服务器地址（Google DNS 8.8.8.8，端口 53，仅用于触发本地路由）
    struct sockaddr_in serverAddr{};
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(53); // DNS 端口（任意端口均可，53 是常用默认值）

    // 解析 IP 地址（兼容 IPv4 字符串直接转换）
    if (inet_pton(AF_INET, "8.8.8.8", &serverAddr.sin_addr) <= 0) {
        // 解析失败时，尝试通过域名解析（可选，增强兼容性）
        struct hostent* host = gethostbyname("8.8.8.8");
        if (host == nullptr || host->h_addr_list[0] == nullptr) {
#ifdef _WIN32
            closesocket(sockfd);
            WSACleanup();
#else
            close(sockfd);
#endif
            return lanIp;
        }
        memcpy(&serverAddr.sin_addr, host->h_addr_list[0], host->h_length);
    }

    // 3. 连接 UDP socket（UDP 是无连接协议，但 connect 仅用于绑定目标地址，不建立实际连接）
    // 此操作会让系统自动选择本地合适的网卡（局域网网卡）和端口，从而获取局域网 IP
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
        // 4. 获取本地绑定的地址（即局域网 IP）
        struct sockaddr_in localAddr{};
        socklen_t localAddrLen = sizeof(localAddr);
        if (getsockname(sockfd, (struct sockaddr*)&localAddr, &localAddrLen) == 0) {
            // 转换网络字节序 IP 到字符串
            char ipBuf[INET_ADDRSTRLEN] = {0};
            if (inet_ntop(AF_INET, &localAddr.sin_addr, ipBuf, sizeof(ipBuf)) != nullptr) {
                lanIp = ipBuf;
            }
        }
    }

    // 5. 清理资源
#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return lanIp;
}


std::string Server::getLanIpv6() {
    std::string lanIpv6 = "::1";
    int sockfd = -1;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return lanIpv6;
    }
#endif

    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return lanIpv6;
    }

    struct sockaddr_in6 serverAddr{};
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(53);

    // 解析 IPv6 地址（跨平台兼容版本）
    if (inet_pton(AF_INET6, "2001:4860:4860::8888", &serverAddr.sin6_addr) <= 0) {
        struct addrinfo hints{}, *res = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;

        int ret = getaddrinfo("ipv6.google.com", nullptr, &hints, &res);
        if (ret != 0 || res == nullptr) {
#ifdef _WIN32
            closesocket(sockfd);
            WSACleanup();
#else
            close(sockfd);
#endif
            return lanIpv6;
        }

        memcpy(&serverAddr.sin6_addr,
               &((struct sockaddr_in6*)res->ai_addr)->sin6_addr,
               sizeof(in6_addr));
        freeaddrinfo(res);
    }

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
        struct sockaddr_in6 localAddr{};
        socklen_t localAddrLen = sizeof(localAddr);
        if (getsockname(sockfd, (struct sockaddr*)&localAddr, &localAddrLen) == 0) {
            uint8_t* addrBytes = localAddr.sin6_addr.s6_addr;
            bool isLinkLocal = (addrBytes[0] == 0xfe && (addrBytes[1] & 0xc0) == 0x80);
            bool isLoopback = (memcmp(addrBytes, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 16) == 0);

            if (!isLinkLocal && !isLoopback) {
                char ipBuf[INET6_ADDRSTRLEN] = {0};
                if (inet_ntop(AF_INET6, &localAddr.sin6_addr, ipBuf, sizeof(ipBuf)) != nullptr) {
                    lanIpv6 = ipBuf;
                }
            }
        }
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return lanIpv6;
}

/**
 * 停止服务器函数
 * 该函数用于安全地关闭服务器，释放相关资源
 * Stop server function
 * This function safely shuts down the server and releases related resources
 */
void Server::stop() {
    // 检查服务器是否已经停止，如果已停止则直接返回
    if (!running_) {
        return;  // 已经停止，避免重复输出
    }
    
    // 设置服务器运行状态为停止
    running_ = false;
    // 输出服务器停止信息
    std::cout << "Server stopped." << std::endl;
    
    // 检查服务器套接字是否有效
    if (serverSocket_ >= 0) {
        // 关闭服务器套接字
        close(serverSocket_);
        // 将套接字描述符重置为无效值
        serverSocket_ = -1;
    }
#ifdef _WIN32
    // 如果是Windows平台，进行Windows套接字清理
    WSACleanup();
#endif
}

/**
 * 处理客户端请求的函数
 * @param clientSocket 客户端套接字描述符
 * @param clientAddress 客户端地址结构指针
 * Handle client request function
 * @param clientSocket Client socket descriptor
 * @param clientAddress Client address structure pointer
 */
void Server::handleClient(int clientSocket, const sockaddr_in *clientAddress) {
    std::time_t now = std::time(nullptr);
    long long timestamp = static_cast<long long>(now) * 1000;

    // 创建缓冲区并初始化为0，用于接收客户端数据
    char buffer[8192] = {0};
    // 从客户端读取数据，读取的字节数存储在bytesRead中
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    // 如果读取字节数小于等于0，表示连接已关闭或出错
    if (bytesRead <= 0) {
        // 关闭客户端套接字并返回
        close(clientSocket);
        return;
    }
    
    // 将接收到的数据转换为字符串
    std::string requestStr(buffer, bytesRead);

    Log::getInstance()->write(getFormattedDate()+" "+getClientIP(clientAddress)+" "+requestStr);

    // 解析HTTP请求
    Request request = parseRequest(requestStr);

    // 创建响应对象
    Response response;
    
    // 处理 OPTIONS 预检请求（CORS）
    if (request.method == "OPTIONS") {
        response.statusCode = 200;
        response.body = "";
        // CORS 头将在 buildResponse 中添加
    } else {
        // 查找路由处理器
        Handler handler = findHandler(request.method, request.path);
        if (handler) {
            // 如果找到处理器，执行处理函数
            try {
                handler(request, response);
            } catch (const std::exception& e) {
                // 捕获异常并返回500错误
                response.statusCode = 500;
                response.error(500,"error: " + std::string(e.what()));
            }
        } else {
            // 如果未找到处理器，返回404错误
            response.statusCode = 404;
            response.error(404, "Resource not found");
        }
    }
    
    // 构建响应字符串并发送给客户端
    std::string responseStr = buildResponse(response);
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);

    // 打印访问日志（类似Apache/Nginx格式）
    std::string clientIP = getClientIP(clientAddress);
    std::lock_guard<std::mutex> lock(logMutex_);
    std::cout << clientIP << " - - [" << getFormattedDate() << "] \"" 
              << request.method << " " << request.path;
    if (!request.queryParams.empty()&&LogParams) {
        std::cout << "?";
        bool first = true;
        for (const auto& param : request.queryParams) {
            if (!first) std::cout << "&";
            std::cout << param.first << "=" << param.second;
            first = false;
        }
    }
    std::cout << " HTTP/1.1\" " << response.statusCode << " " 
              << response.body.length();

    std::time_t end_time = std::time(nullptr);
    long long end_timestamp = static_cast<long long>(end_time) * 1000;
    std::cout << end_timestamp - timestamp << "ms" << std::endl;

    close(clientSocket);
}

/**
 * 解析HTTP请求字符串，将其解析为Request结构体
 * @param requestStr HTTP请求字符串
 * @return 解析后的Request对象
 */
Request Server::parseRequest(const std::string& requestStr) {
    // 创建Request对象和字符串流
    Request request;
    std::istringstream iss(requestStr);
    std::string line;
    
    // 解析请求行（第一行）
    if (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        // 提取方法和路径
        lineStream >> request.method >> request.path;
        
        // 分离路径和查询参数
        size_t queryPos = request.path.find('?');
        if (queryPos != std::string::npos) {
            std::string query = request.path.substr(queryPos + 1);
            request.queryParams = parseQueryParams(query);
            // 解析查询参数
            request.path = request.path.substr(0, queryPos);
        }
        // 更新路径为不包含查询参数的部分
    }
    
    // 解析头部和body
    bool inBody = false;
    std::string body;
    while (std::getline(iss, line)) {  // 标记是否进入body部分
        if (line.empty() || line == "\r") {
            inBody = true;
        // 空行或\r表示头部结束，body开始
            continue;
        }
        
        if (!inBody) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
            // 解析头部字段
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                // 提取键值对

                // 去除空格和\r
                key.erase(0, key.find_first_not_of(" \t\r"));
                key.erase(key.find_last_not_of(" \t\r") + 1);
                value.erase(0, value.find_first_not_of(" \t\r"));
                value.erase(value.find_last_not_of(" \t\r") + 1);
                
                request.headers[key] = value;
//                std::cout<<"key: "<<key<<" value: "<<value<<std::endl;
            }
        } else {
            body += line;
            if (!iss.eof()) body += "\n";
        }
    }
    
    request.body = body;
    request.parseBody();
    return request;
}

/**
 * 构建HTTP响应字符串
 * @param response 包含状态码、头部和响应体的Response对象
 * @return 构建好的HTTP响应字符串
 */
std::string Server::buildResponse(const Response& response) {
//    std::cout<<"Build Response"<<std::endl;
//    std::cout<<response.statusCode<<std::endl;
    std::ostringstream oss;  // 使用字符串流构建响应
    
    // 添加状态行，包括HTTP版本、状态码和状态描述
    oss << "HTTP/1.1 " << response.statusCode << " ";
    switch (response.statusCode) {
        case 200: oss << "OK"; break;      // 200 OK - 请求成功
// 200 OK - Request successful
        case 201: oss << "Created"; break;  // 201 Created - 资源创建成功
// 201 Created - Resource created successfully
        case 400: oss << "Bad Request"; break;  // 400 Bad Request - 客户端请求错误
// 400 Bad Request - Client request error
        case 404: oss << "Not Found"; break;    // 404 Not Found - 资源未找到
// 404 Not Found - Resource not found
        case 500: oss << "Internal Server Error"; break;  // 500 Internal Server Error - 服务器内部错误
// 500 Internal Server Error - Server internal error
        default: oss << "Unknown"; break;   // 未知状态码
    }
    oss << "\r\n";  // HTTP协议使用\r\n作为换行符
    
    // 添加 CORS 头（跨域支持）
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n";
    oss << "Access-Control-Max-Age: 86400\r\n";  // 预检请求缓存24小时
    
    // 添加其他头部
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "Content-Length: " << response.body.length() << "\r\n";
    oss << "\r\n";
    oss << response.body;

//    std::cout<<oss.str()<<std::endl;
    
    return oss.str();
}

/**
 * 查找并返回与给定方法和路径匹配的处理程序
 * @param method HTTP方法（如"GET"、"POST"等）
 * @param path 请求的URL路径
 * @return 匹配的Handler对象，如果未找到则返回nullptr
 */
Handler Server::findHandler(const std::string& method, const std::string& path) {
    // 检查是否存在对应方法的路由映射
    if (routes_.find(method) != routes_.end()) {
        const auto& methodRoutes = routes_[method];  // 获取该方法的所有路由
        
        // 精确匹配检查
        if (methodRoutes.find(path) != methodRoutes.end()) {
            return methodRoutes.at(path);  // 返回精确匹配的处理程序
        }
    }
    
    return nullptr;  // 未找到匹配的处理程序，返回空指针
}

#include <string>

/**
 * URL解码函数
 * 用于解码URL中的特殊字符，如将%编码的字符转换为原始字符，将+转换为空格
 * @param value 需要解码的URL字符串
 * @return 解码后的字符串
 */
std::string Server::urlDecode(const std::string& value) {
    std::string result;
    result.reserve(value.size());  // 预分配内存，提升效率

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            // 处理查询参数中的空格（+ 对应空格）
            result += ' ';
        } else if (value[i] == '%' && i + 2 < value.size()) {
            // 提取 % 后的两个十六进制字符
            char c1 = value[i + 1];
            char c2 = value[i + 2];

            // 检查是否为有效的十六进制字符（0-9, a-f, A-F）
            auto isHexChar = [](char c) {
                return (c >= '0' && c <= '9') ||
                       (c >= 'a' && c <= 'f') ||
                       (c >= 'A' && c <= 'F');
            };

            if (!isHexChar(c1) || !isHexChar(c2)) {
                // 无效的十六进制字符，保留 % 并继续
                result += value[i];
                continue;
            }

            // 转换十六进制字符串为字节值（避免使用 stoi 减少异常）
            auto hexToByte = [](char c) {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
                return 10 + (c - 'A'); // 大写字母
            };

            unsigned char byte = (hexToByte(c1) << 4) | hexToByte(c2);
            result += static_cast<char>(byte);
            i += 2;  // 跳过已处理的两个字符
        } else {
            // 普通字符直接添加（包括 ASCII 和 UTF-8 多字节的后续字节）
            result += value[i];
        }
    }

    return result;
}

/**
 * 解析URL查询参数字符串，将其转换为键值对映射
 * Parse URL query parameter string and convert to key-value pair map
 * @param query 包含查询参数的字符串，格式为"key1=value1&key2=value2..."
 * @return 包含解析后的键值对的map，其中值经过URL解码
 */
std::map<std::string, std::string> Server::parseQueryParams(const std::string& query) {
    std::map<std::string, std::string> params;  // 存储解析后的键值对
    std::istringstream iss(query);              // 使用字符串流处理查询字符串
    std::string pair;                          // 存储单个键值对字符串
    
    // 按 '&' 分割查询字符串，逐个处理键值对
    while (std::getline(iss, pair, '&')) {
        size_t pos = pair.find('=');           // 查找 '=' 的位置
        if (pos != std::string::npos) {        // 如果找到 '='
            std::string key = pair.substr(0, pos);      // 提取键
            std::string value = pair.substr(pos + 1);   // 提取值
            params[key] = urlDecode(value);    // 对值进行URL解码并存入map
        }
    }
    
    return params;
}  // 返回解析后的参数map

void Server::printRegisteredRoutes() const {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Registered API Routes:" << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (routes_.empty()) {
        std::cout << "  (No routes registered)" << std::endl;
    } else {
        // 按HTTP方法分组显示
        const std::vector<std::string> methods = {"GET", "POST", "PUT", "DELETE"};
        
        for (const auto& method : methods) {
            if (routes_.find(method) != routes_.end()) {
                const auto& methodRoutes = routes_.at(method);
                if (!methodRoutes.empty()) {
                    std::cout << "\n  " << method << ":" << std::endl;
                    for (const auto& route : methodRoutes) {
                        std::cout << "    " << route.first << std::endl;
                    }
                }
            }
        }
    }
    
    std::cout << "========================================" << std::endl;
}

std::string getFormattedDate() {
    std::time_t now = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(timeInfo, "%d/%b/%Y:%H:%M:%S");
    
    // 获取时区偏移（简化版，显示+0800格式）
#ifdef _WIN32
    // Windows上的时区处理
    TIME_ZONE_INFORMATION tzInfo;
    GetTimeZoneInformation(&tzInfo);
    int offset = -tzInfo.Bias / 60;  // Bias是以分钟为单位，需要转换为小时
#else
    // Linux上的时区处理
    int offset = timeInfo->tm_gmtoff / 3600;
#endif
    
    oss << " " << std::showpos << std::setfill('0') << std::setw(3) 
        << offset << "00";
    
    return oss.str();
}

std::string Server::getClientIP(const sockaddr_in *clientAddress) {
    if (!clientAddress) {
        return "unknown";
    }
    
    char ipStr[INET_ADDRSTRLEN];
#ifdef _WIN32
    InetNtopA(AF_INET, &(clientAddress->sin_addr), ipStr, INET_ADDRSTRLEN);
#else
    inet_ntop(AF_INET, &(clientAddress->sin_addr), ipStr, INET_ADDRSTRLEN);
#endif
    
    return std::string(ipStr);
}

Server* Server::getInstance() {
    return instance_;
}

