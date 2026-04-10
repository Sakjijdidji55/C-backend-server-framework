/**
 * @file ApiSender.cpp
 * @brief 外部 API 请求类实现（HTTP 客户端，异步 + 回调）
 * @notice HTTP 使用原生 socket；HTTPS 在 Windows 用 WinHTTP，在 Unix 用 OpenSSL（与 CMake 中依赖一致）。
 */
#include "../include/ApiSender.h"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#endif
#define close_sock(s) closesocket(s)
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define close_sock(s) close(s)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;
#endif

namespace {

struct ParsedUrl {
    bool https = false;
    std::string host;
    int port = 80;
    std::string path;
    bool ok = false;
};

ParsedUrl parseUrl(const std::string& url) {
    ParsedUrl r;
    if (url.rfind("https://", 0) == 0) {
        r.https = true;
        r.port = 443;
    }
    else if (url.rfind("http://", 0) == 0) {
        r.https = false;
        r.port = 80;
    }
    else {
        return r;
    }

    const size_t schemeLen = r.https ? 8u : 7u;
    const size_t pathStart = url.find('/', schemeLen);
    std::string hostPort;
    if (pathStart == std::string::npos) {
        hostPort = url.substr(schemeLen);
        r.path = "/";
    }
    else {
        hostPort = url.substr(schemeLen, pathStart - schemeLen);
        r.path = url.substr(pathStart);
    }

    const size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        r.host = hostPort.substr(0, colon);
        try {
            r.port = std::stoi(hostPort.substr(colon + 1));
        }
        catch (...) {
            r.port = r.https ? 443 : 80;
        }
    }
    else {
        r.host = hostPort;
    }

    r.ok = !r.host.empty();
    return r;
}

std::string httpVerb(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DEL: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        default: return "GET";
    }
}

#ifdef _WIN32
std::wstring utf8ToWide(const std::string& s) {
    if (s.empty())
        return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (n <= 0)
        return std::wstring();
    std::wstring w(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

ApiResponse doRequestHttpsWinHttp(HttpMethod method, const ParsedUrl& pu,
                                const std::map<std::string, std::string>& headers,
                                const std::string& body) {
    ApiResponse resp;
    const std::wstring userAgent = L"AIRI-CBSF/1.0";
    HINTERNET hSession = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        resp.error = "WinHttpOpen failed: " + std::to_string(GetLastError());
        return resp;
    }

    const std::wstring hostW = utf8ToWide(pu.host);
    const std::wstring pathW = utf8ToWide(pu.path.empty() ? "/" : pu.path);
    const std::wstring verbW = utf8ToWide(httpVerb(method));

    HINTERNET hConnect = WinHttpConnect(hSession, hostW.c_str(), static_cast<INTERNET_PORT>(pu.port), 0);
    if (!hConnect) {
        resp.error = "WinHttpConnect failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hSession);
        return resp;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, verbW.c_str(), pathW.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        resp.error = "WinHttpOpenRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    bool hasContentLength = false;
    for (const auto& h : headers) {
        std::string k = h.first;
        std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (k == "content-length")
            hasContentLength = true;
        const std::string line = h.first + ": " + h.second;
        const std::wstring wline = utf8ToWide(line);
        if (!WinHttpAddRequestHeaders(hRequest, wline.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
            resp.error = "WinHttpAddRequestHeaders failed: " + std::to_string(GetLastError());
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return resp;
        }
    }
    if (!body.empty() && !hasContentLength) {
        const std::string cl = "Content-Length: " + std::to_string(body.size());
        const std::wstring wcl = utf8ToWide(cl);
        WinHttpAddRequestHeaders(hRequest, wcl.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    LPVOID optPtr = body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data();
    const DWORD optLen = body.empty() ? 0 : static_cast<DWORD>(body.size());
    const DWORD totalLen = optLen;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, optPtr, optLen, totalLen, 0)) {
        resp.error = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        resp.error = "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    DWORD statusCode = 0;
    DWORD sz = sizeof(statusCode);
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz, WINHTTP_NO_HEADER_INDEX)) {
        resp.statusCode = static_cast<int>(statusCode);
        resp.success = true;
    }

    std::string rawBody;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &avail))
            break;
        if (avail == 0)
            break;
        std::string chunk(avail, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(hRequest, &chunk[0], avail, &read))
            break;
        chunk.resize(read);
        rawBody += chunk;
    }
    resp.body = std::move(rawBody);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return resp;
}
#else
ApiResponse doRequestHttpsOpenSsl(HttpMethod method, const ParsedUrl& pu,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& body) {
    ApiResponse resp;
    static bool sslInited = false;
    if (!sslInited) {
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        sslInited = true;
    }

    struct addrinfo hints {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    const std::string portStr = std::to_string(pu.port);
    if (getaddrinfo(pu.host.c_str(), portStr.c_str(), &hints, &res) != 0 || !res) {
        resp.error = "getaddrinfo failed: " + pu.host;
        return resp;
    }

    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        resp.error = "socket failed";
        freeaddrinfo(res);
        return resp;
    }
    if (connect(sock, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) < 0) {
        resp.error = "connect failed: " + pu.host + ":" + portStr;
        close_sock(sock);
        freeaddrinfo(res);
        return resp;
    }
    freeaddrinfo(res);

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        resp.error = "SSL_CTX_new failed";
        close_sock(sock);
        return resp;
    }
    SSL_CTX_set_default_verify_paths(ctx);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        resp.error = "SSL_new failed";
        SSL_CTX_free(ctx);
        close_sock(sock);
        return resp;
    }
    SSL_set_tlsext_host_name(ssl, pu.host.c_str());
    SSL_set_fd(ssl, static_cast<int>(sock));
    if (SSL_connect(ssl) <= 0) {
        resp.error = "SSL_connect failed";
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close_sock(sock);
        return resp;
    }

    std::ostringstream req;
    req << httpVerb(method) << " " << pu.path << " HTTP/1.1\r\n";
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

    const std::string reqStr = req.str();
    if (SSL_write(ssl, reqStr.data(), static_cast<int>(reqStr.size())) <= 0) {
        resp.error = "SSL_write failed";
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close_sock(sock);
        return resp;
    }

    std::string raw;
    char buf[4096];
    int n;
    while ((n = SSL_read(ssl, buf, sizeof(buf))) > 0)
        raw.append(buf, buf + n);

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close_sock(sock);

    const size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        resp.body = raw.substr(headerEnd + 4);
        const std::string statusLine = raw.substr(0, raw.find("\r\n"));
        if (statusLine.size() >= 12 && statusLine.substr(0, 7) == "HTTP/1.") {
            try {
                resp.statusCode = std::stoi(statusLine.substr(9, 3));
                resp.success = true;
            }
            catch (...) {}
        }
    }
    else {
        resp.error = "invalid TLS response";
    }
    return resp;
}
#endif

} // namespace

ApiSender::ApiSender(size_t numWorkers) : pool_(std::make_unique<ThreadPool>(numWorkers)) {}

ApiSender::~ApiSender() = default;

std::string ApiSender::methodToString(HttpMethod method) {
    return httpVerb(method);
}

ApiResponse ApiSender::doRequest(HttpMethod method, const std::string& url,
                                  const std::map<std::string, std::string>& headers,
                                  const std::string& body) {
    ApiResponse resp;
    const ParsedUrl pu = parseUrl(url);
    if (!pu.ok) {
        resp.error = "Invalid URL (need http:// or https://)";
        return resp;
    }

    if (pu.https) {
#ifdef _WIN32
        return doRequestHttpsWinHttp(method, pu, headers, body);
#else
        return doRequestHttpsOpenSsl(method, pu, headers, body);
#endif
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        resp.error = "WSAStartup failed";
        return resp;
    }
#endif
    struct addrinfo hints {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    const std::string portStr = std::to_string(pu.port);
    int gret = getaddrinfo(pu.host.c_str(), portStr.c_str(), &hints, &res);
    if (gret != 0 || !res) {
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

    const std::string methodStr = methodToString(method);
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

    const std::string reqStr = req.str();
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
        const std::string statusLine = raw.substr(0, raw.find("\r\n"));
        if (statusLine.size() >= 12 && statusLine.substr(0, 7) == "HTTP/1.") {
            try {
                resp.statusCode = std::stoi(statusLine.substr(9, 3));
                resp.success = true;
            }
            catch (...) {}
        }
    }
    else {
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
    request(HttpMethod::DEL, url, headers, "", std::move(callback));
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
