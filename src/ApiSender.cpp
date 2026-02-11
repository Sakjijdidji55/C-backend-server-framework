/**
 * @file ApiSender.cpp
 * @brief 外部 API 请求类实现（HTTP 客户端，异步 + 回调）
 */
#include "../include/ApiSender.h"
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#define close_sock(s) closesocket(s)
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define close_sock(s) close(s)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;
#endif

namespace {

struct ParsedUrl {
    std::string host;
    int port = 80;
    std::string path;
    bool ok = false;
};

ParsedUrl parseUrl(const std::string& url) {
    ParsedUrl r;
    if (url.size() < 8 || (url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://")) {
        r.path = "/";
        return r;
    }
    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) return r;
    schemeEnd += 3;
    if (url.substr(0, schemeEnd - 3) == "https") {
        r.port = 443;
        r.ok = false;
        return r;
    }
    size_t pathStart = url.find('/', schemeEnd);
    std::string hostPort;
    if (pathStart == std::string::npos) {
        hostPort = url.substr(schemeEnd);
        r.path = "/";
    } else {
        hostPort = url.substr(schemeEnd, pathStart - schemeEnd);
        r.path = url.substr(pathStart);
    }
    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        r.host = hostPort.substr(0, colon);
        try {
            r.port = std::stoi(hostPort.substr(colon + 1));
        } catch (...) {
            r.port = 80;
        }
    } else {
        r.host = hostPort;
        r.port = 80;
    }
    r.ok = !r.host.empty();
    return r;
}

} // namespace

ApiSender::ApiSender(size_t numWorkers) : pool_(std::make_unique<ThreadPool>(numWorkers)) {}

ApiSender::~ApiSender() = default;

std::string ApiSender::methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        default: return "GET";
    }
}

ApiResponse ApiSender::doRequest(HttpMethod method, const std::string& url,
                                  const std::map<std::string, std::string>& headers,
                                  const std::string& body) {
    ApiResponse resp;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        resp.error = "WSAStartup failed";
        return resp;
    }
#endif
    ParsedUrl pu = parseUrl(url);
    if (!pu.ok) {
        resp.error = "Invalid URL or HTTPS not supported (use http://)";
#ifdef _WIN32
        WSACleanup();
#endif
        return resp;
    }
    struct addrinfo hints {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    std::string portStr = std::to_string(pu.port);
    int ret = getaddrinfo(pu.host.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        resp.error = "getaddrinfo failed: " + pu.host;
#ifdef _WIN32
        WSACleanup();
#endif
        return resp;
    }
    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        resp.error = "socket failed";
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return resp;
    }
    if (connect(sock, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) == SOCKET_ERROR) {
        resp.error = "connect failed: " + pu.host + ":" + portStr;
        close_sock(sock);
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return resp;
    }
    freeaddrinfo(res);

    std::string methodStr = methodToString(method);
    std::ostringstream req;
    req << methodStr << " " << pu.path << " HTTP/1.1\r\n";
    req << "Host: " << pu.host << "\r\n";
    req << "Connection: close\r\n";
    bool hasContentLength = false;
    for (const auto& h : headers) {
        std::string k = h.first;
        if (k.size() >= 15 && std::equal(k.begin(), k.begin() + 15, "Content-Length")) hasContentLength = true;
        req << k << ": " << h.second << "\r\n";
    }
    if (!body.empty() && !hasContentLength)
        req << "Content-Length: " << body.size() << "\r\n";
    req << "\r\n";
    req << body;

    std::string reqStr = req.str();
    if (send(sock, reqStr.c_str(), static_cast<int>(reqStr.size()), 0) == SOCKET_ERROR) {
        resp.error = "send failed";
        close_sock(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return resp;
    }

    std::string raw;
    char buf[4096];
    int n;
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0)
        raw.append(buf, buf + n);
    close_sock(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) headerEnd = raw.find("\n\n");
    if (headerEnd != std::string::npos) {
        resp.body = raw.substr(headerEnd + (raw.substr(headerEnd, 4) == "\r\n\r\n" ? 4u : 2u));
        std::string statusLine = raw.substr(0, raw.find("\r\n"));
        if (statusLine.size() >= 12 && statusLine.substr(0, 7) == "HTTP/1.") {
            try {
                resp.statusCode = std::stoi(statusLine.substr(9, 3));
                resp.success = true;
            } catch (...) {}
        }
    } else {
        resp.error = "invalid response";
    }
    return resp;
}

void ApiSender::request(HttpMethod method, const std::string& url,
                        const std::map<std::string, std::string>& headers,
                        const std::string& body, ApiCallback callback) {
    auto cb = callback;
    pool_->addTask([method, url, headers, body, cb]() {
        ApiResponse r = ApiSender::doRequest(method, url, headers, body);
        if (cb) cb(r);
    });
}

void ApiSender::get(const std::string& url,
                    const std::map<std::string, std::string>& headers,
                    ApiCallback callback) {
    request(HttpMethod::GET, url, headers, "", std::move(callback));
}

void ApiSender::post(const std::string& url,
                     const std::map<std::string, std::string>& headers,
                     const std::string& body, ApiCallback callback) {
    request(HttpMethod::POST, url, headers, body, std::move(callback));
}

void ApiSender::put(const std::string& url,
                    const std::map<std::string, std::string>& headers,
                    const std::string& body, ApiCallback callback) {
    request(HttpMethod::PUT, url, headers, body, std::move(callback));
}

void ApiSender::del(const std::string& url,
                    const std::map<std::string, std::string>& headers,
                    ApiCallback callback) {
    request(HttpMethod::DELETE, url, headers, "", std::move(callback));
}

void ApiSender::patch(const std::string& url,
                      const std::map<std::string, std::string>& headers,
                      const std::string& body, ApiCallback callback) {
    request(HttpMethod::PATCH, url, headers, body, std::move(callback));
}

ApiResponse ApiSender::requestSync(HttpMethod method, const std::string& url,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& body) {
    return doRequest(method, url, headers, body);
}
