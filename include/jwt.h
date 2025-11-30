//
// Created by HP on 2025/11/13.
//

#ifndef FLIGHTSERVER_JWT_H
#define FLIGHTSERVER_JWT_H
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <climits>
#include <cctype>
#include <algorithm>
#include <utility>
#include <map>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <iomanip>
#include <random>

// JWT类，用于生成和验证JSON Web Token
// JWT class for generating and verifying JSON Web Tokens
class JWT {
public:
    // 加密算法枚举
    // Encryption algorithm enumeration
    enum class Algorithm {
        HS256
    };

    // 构造函数，设置过期时间和密钥路径
    // Constructor, set expiration time and key path
    explicit JWT(const std::string& expiresIn="", const std::string& jwtRsaPrivateKeyPath="");
    // 构造函数，设置密钥和默认过期时间（秒）
    // Constructor, set secret key and default TTL (seconds)
    explicit JWT(std::string secret, long long ttlSeconds = 3600);

    void setSecret(const std::string& secret);
    void loadSecretFromFile(const std::string& path);
    void setDefaultTTL(long long ttlSeconds);

    // 生成令牌
    // Generate token
    [[nodiscard]] std::string generateToken(const std::map<std::string, std::string>& customClaims = {},
                                            long long ttlSeconds = -1) const;

    // 验证令牌
    // Verify token
    [[nodiscard]] bool verifyToken(const std::string& token,
                                   std::string* payloadJson = nullptr) const;

    // 获取载荷中的声明
    // Get claims from payload
    [[nodiscard]] std::optional<std::map<std::string, std::string>> parseClaims(const std::string& token) const;

    [[nodiscard]] Algorithm algorithm() const;

    static std::string encrypt_password(const std::string &password);

    static bool verify_password(const std::string& password, const std::string& stored_hash) ;

    static JWT* getInstance();

private:
    // Base64 URL编码
    // Base64 URL encode
    static std::string base64UrlEncode(const std::string& data);
    // Base64 URL解码
    // Base64 URL decode
    static std::string base64UrlDecode(const std::string& data);

    // SHA-256哈希
    // SHA-256 hash
    static std::string sha256(const std::string& data);
    // HMAC-SHA256哈希
    // HMAC-SHA256 hash
    static std::string hmacSha256(const std::string& data, const std::string& key);

    static std::string buildHeader();
    static std::string buildPayload(const std::map<std::string, std::string>& customClaims,
                                    long long issuedAt,
                                    std::optional<long long> expiresAt);
    static std::string escapeJson(const std::string& input);

    static std::vector<std::string> splitToken(const std::string& token);
    static std::optional<long long> extractNumericClaim(const std::string& payload, const std::string& claimKey);

    static long long parseTTL(const std::string& ttlSpec);
    static std::map<std::string, std::string> parseJsonObject(const std::string& json);
    static std::string unescapeJson(const std::string& input);

    // 原生SHA-256实现（返回原始字节）
    static std::vector<uint8_t> sha256_bytes(const std::vector<uint8_t>& data) ;

    // HMAC-SHA256实现（基于sha256_bytes）
    static std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& key,const std::vector<uint8_t>& data) ;

    // 生成随机盐值（16字节，足够安全）
    static std::vector<uint8_t> generate_salt() ;

    // 字节转十六进制字符串（便于存储）
    static std::string bytes_to_hex(const std::vector<uint8_t>& bytes);

    // 补充：十六进制字符串转字节数组（解析盐值和哈希时需要）
    static std::vector<uint8_t> hex_to_bytes(const std::string& hex) ;

    // 常量时间比较（防时序攻击）：无论内容是否相同，比较时间一致
    static bool constant_time_compare(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) ;

    std::string secret_;
    Algorithm algorithm_;
    long long defaultTTL_;
    static JWT* instance_;
};

#endif //FLIGHTSERVER_JWT_H
