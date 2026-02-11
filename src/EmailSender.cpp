/**
 * @file EmailSender.cpp
 * @brief 邮件发送类实现（SMTP 客户端，单例模式）
 */
#include "../include/EmailSender.h"
#include <cstring>
#include <sstream>
#include <algorithm>
#include <cctype>

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

EmailSender* EmailSender::instance_ = nullptr;

EmailSender* EmailSender::getInstance() {
    static EmailSender inst;
    instance_ = &inst;
    return instance_;
}

void EmailSender::init(const std::string& smtpHost, unsigned short smtpPort,
                       const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    smtpHost_ = smtpHost;
    smtpPort_ = smtpPort;
    username_ = username;
    password_ = password;
    lastError_.clear();
}

std::string EmailSender::base64Encode(const std::string& raw) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    for (; i + 3 <= raw.size(); i += 3) {
        unsigned int v = (static_cast<unsigned char>(raw[i]) << 16)
                         | (static_cast<unsigned char>(raw[i + 1]) << 8)
                         | static_cast<unsigned char>(raw[i + 2]);
        out += tbl[(v >> 18) & 63];
        out += tbl[(v >> 12) & 63];
        out += tbl[(v >> 6) & 63];
        out += tbl[v & 63];
    }
    if (i < raw.size()) {
        unsigned int v = static_cast<unsigned char>(raw[i]) << 16;
        if (i + 1 < raw.size()) v |= static_cast<unsigned char>(raw[i + 1]) << 8;
        out += tbl[(v >> 18) & 63];
        out += tbl[(v >> 12) & 63];
        out += (i + 1 < raw.size()) ? tbl[(v >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

bool EmailSender::smtpConnect(int* sockOut) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        lastError_ = "WSAStartup failed";
        return false;
    }
#endif
    struct addrinfo hints {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    std::string portStr = std::to_string(smtpPort_);
    int ret = getaddrinfo(smtpHost_.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        lastError_ = "getaddrinfo failed: " + smtpHost_;
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }
    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        lastError_ = "socket failed";
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }
    if (connect(sock, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) == SOCKET_ERROR) {
        lastError_ = "connect failed: " + smtpHost_ + ":" + portStr;
        close_sock(sock);
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }
    freeaddrinfo(res);
    *sockOut = static_cast<int>(sock);
    return true;
}

bool EmailSender::smtpCommandRecv(int sock, std::string* outLine) {
    char buf[512];
    std::string line;
    while (true) {
        int n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) return false;
        buf[n] = '\0';
        line += buf;
        if (line.size() >= 4 && line[3] == ' ') break;
        if (line.find("\r\n") != std::string::npos) break;
    }
    size_t end = line.find("\r\n");
    if (end != std::string::npos) line.resize(end);
    if (outLine) *outLine = line;
    return true;
}

bool EmailSender::smtpCommand(int sock, const std::string& cmd, int expectCode) {
    std::string sendStr = cmd + "\r\n";
    if (send(sock, sendStr.c_str(), static_cast<int>(sendStr.size()), 0) == SOCKET_ERROR)
        return false;
    std::string line;
    if (!smtpCommandRecv(sock, &line)) return false;
    int code = 0;
    if (line.size() >= 3) code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
    return code == expectCode;
}

bool EmailSender::sendMailImpl(const std::string& from, const std::vector<std::string>& toList,
                               const std::string& subject, const std::string& body) {
    if (smtpHost_.empty() || username_.empty()) {
        lastError_ = "EmailSender not initialized. Call init() first.";
        return false;
    }
    int sock = -1;
    if (!smtpConnect(&sock)) return false;

    bool ok = true;
    std::string line;

    if (!smtpCommandRecv(sock, &line) || line.size() < 3 || line.substr(0, 3) != "220") {
        lastError_ = "SMTP greeting failed: " + line;
        close_sock(sock);
        return false;
    }

    if (!smtpCommand(sock, "EHLO localhost", 250)) {
        if (!smtpCommand(sock, "HELO localhost", 250)) {
            lastError_ = "EHLO/HELO failed";
            close_sock(sock);
            return false;
        }
    }

    if (!smtpCommand(sock, "AUTH LOGIN", 334)) {
        lastError_ = "AUTH LOGIN failed";
        close_sock(sock);
        return false;
    }
    if (!smtpCommand(sock, base64Encode(username_), 334)) {
        lastError_ = "SMTP username rejected";
        close_sock(sock);
        return false;
    }
    if (!smtpCommand(sock, base64Encode(password_), 235)) {
        lastError_ = "SMTP password rejected";
        close_sock(sock);
        return false;
    }

    if (!smtpCommand(sock, "MAIL FROM:<" + from + ">", 250)) {
        lastError_ = "MAIL FROM failed";
        close_sock(sock);
        return false;
    }
    for (const auto& to : toList) {
        std::string t = to;
        t.erase(std::remove_if(t.begin(), t.end(), ::isspace), t.end());
        if (t.empty()) continue;
        if (!smtpCommand(sock, "RCPT TO:<" + t + ">", 250)) {
            lastError_ = "RCPT TO failed: " + t;
            ok = false;
            break;
        }
    }
    if (!ok) {
        close_sock(sock);
        return false;
    }
    if (!smtpCommand(sock, "DATA", 354)) {
        lastError_ = "DATA failed";
        close_sock(sock);
        return false;
    }

    std::string data = "From: " + from + "\r\nTo: " + (toList.empty() ? "" : toList[0]);
    for (size_t i = 1; i < toList.size(); ++i) data += ", " + toList[i];
    data += "\r\nSubject: " + subject + "\r\nContent-Type: text/plain; charset=UTF-8\r\n\r\n";
    data += body;
    data += "\r\n.";
    std::string sendStr = data + "\r\n";
    if (send(sock, sendStr.c_str(), static_cast<int>(sendStr.size()), 0) == SOCKET_ERROR) {
        lastError_ = "send DATA failed";
        close_sock(sock);
        return false;
    }
    if (!smtpCommandRecv(sock, &line) || line.size() < 3 || line.substr(0, 3) != "250") {
        lastError_ = "DATA response failed: " + line;
        close_sock(sock);
        return false;
    }
    smtpCommand(sock, "QUIT", 221);
    close_sock(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

bool EmailSender::sendMail(const std::string& from, const std::string& to,
                           const std::string& subject, const std::string& body) {
    std::vector<std::string> toList;
    std::string t;
    for (char c : to) {
        if (c == ',' || c == ';') {
            if (!t.empty()) { toList.push_back(t); t.clear(); }
        } else if (!std::isspace(static_cast<unsigned char>(c)))
            t += c;
    }
    if (!t.empty()) toList.push_back(t);
    return sendMail(from, toList, subject, body);
}

bool EmailSender::sendMail(const std::string& from, const std::vector<std::string>& toList,
                           const std::string& subject, const std::string& body) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sendMailImpl(from, toList, subject, body);
}
