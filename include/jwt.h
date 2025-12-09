//
// Created by HP on 2025/11/13.
//

/**
 * @file jwt.h
 * @brief JWT令牌生成和验证类头文件
 * @brief JWT Token Generation and Verification Class Header File
 */
#ifndef CBSF_JWT_H
#define CBSF_JWT_H
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
#include "JsonValue.h"

/**
 * @class JWT
 * @brief JSON Web Token处理类
 * @brief JSON Web Token Processing Class
 * @note 提供JWT令牌的生成、验证和密码加密等功能
 * @note Provides JWT token generation, verification, and password encryption functionality
 */
class JWT {
public:
    /**
     * @enum Algorithm
     * @brief 加密算法枚举
     * @brief Encryption algorithm enumeration
     */
    enum class Algorithm {
        HS256  ///< HMAC-SHA256算法 (HMAC-SHA256 algorithm)
    };

    /**
     * @brief 构造函数，设置过期时间和密钥路径
     * @brief Constructor, set expiration time and key path
     * @param expiresIn 过期时间规格
     * @param jwtRsaPrivateKeyPath RSA私钥文件路径
     */
    explicit JWT(const std::string& expiresIn="", const std::string& jwtRsaPrivateKeyPath="");
    
    /**
     * @brief 构造函数，设置密钥和默认过期时间（秒）
     * @brief Constructor, set secret key and default TTL (seconds)
     * @param secret JWT签名密钥
     * @param ttlSeconds 默认令牌过期时间（秒），默认3600秒（1小时）
     */
    explicit JWT(std::string secret, long long ttlSeconds = 3600);

    /**
     * @brief 设置JWT签名密钥
     * @brief Set JWT signing secret key
     * @param secret 要设置的密钥
     */
    void setSecret(const std::string& secret);
    
    /**
     * @brief 从文件加载密钥
     * @brief Load secret key from file
     * @param path 密钥文件路径
     */
    void loadSecretFromFile(const std::string& path);
    
    /**
     * @brief 设置默认令牌过期时间
     * @brief Set default token expiration time
     * @param ttlSeconds 过期时间（秒）
     */
    void setDefaultTTL(long long ttlSeconds);

    /**
     * @brief 生成JWT令牌
     * @brief Generate JWT token
     * @param customClaims 自定义声明（键值对）
     * @param ttlSeconds 令牌过期时间（秒），-1表示使用默认值
     * @return 生成的JWT令牌
     */
    [[nodiscard]] std::string generateToken(const std::map<std::string, std::string>& customClaims = {},
                                            long long ttlSeconds = -1) const;

    /**
     * @brief 验证JWT令牌
     * @brief Verify JWT token
     * @param token 要验证的令牌
     * @param payloadJson 可选输出参数，用于存储令牌载荷的JSON字符串
     * @return 验证成功返回true，失败返回false
     */
    [[nodiscard]] bool verifyToken(const std::string& token,
                                   std::string* payloadJson = nullptr) const;

    /**
     * @brief 获取令牌载荷中的声明
     * @brief Get claims from token payload
     * @param token JWT令牌
     * @return 包含声明的映射，如果解析失败则返回std::nullopt
     */
    [[nodiscard]] std::optional<std::map<std::string, std::string>> parseClaims(const std::string& token) const;

    /**
     * @brief 获取当前使用的加密算法
     * @brief Get current encryption algorithm
     * @return 算法枚举值
     */
    [[nodiscard]] Algorithm algorithm() const;

    /**
     * @brief 加密密码
     * @brief Encrypt password
     * @param password 原始密码
     * @return 加密后的密码哈希
     */
    static std::string encrypt_password(const std::string &password);

    /**
     * @brief 验证密码
     * @brief Verify password
     * @param password 待验证的原始密码
     * @param stored_hash 存储的密码哈希值
     * @return 密码匹配返回true，否则返回false
     */
    static bool verify_password(const std::string& password, const std::string& stored_hash) ;

    /**
     * @brief 获取JWT单例实例
     * @brief Get JWT singleton instance
     * @return JWT类实例指针
     */
    static JWT* getInstance();

private:
    /**
     * @brief Base64 URL编码
     * @brief Base64 URL encode
     * @param data 要编码的数据
     * @return 编码后的字符串
     */
    static std::string base64UrlEncode(const std::string& data);
    
    /**
     * @brief Base64 URL解码
     * @brief Base64 URL decode
     * @param data 要解码的数据
     * @return 解码后的字符串
     */
    static std::string base64UrlDecode(const std::string& data);

    /**
     * @brief SHA-256哈希
     * @brief SHA-256 hash
     * @param data 要哈希的数据
     * @return 哈希后的字符串
     */
    static std::string sha256(const std::string& data);
    
    /**
     * @brief HMAC-SHA256哈希
     * @brief HMAC-SHA256 hash
     * @param data 要哈希的数据
     * @param key 密钥
     * @return 哈希后的字符串
     */
    static std::string hmacSha256(const std::string& data, const std::string& key);

    /**
     * @brief 构建JWT头部
     * @brief Build JWT header
     * @return 头部的Base64 URL编码字符串
     */
    static std::string buildHeader();
    
    /**
     * @brief 构建JWT载荷
     * @brief Build JWT payload
     * @param customClaims 自定义声明
     * @param issuedAt 令牌发行时间戳
     * @param expiresAt 令牌过期时间戳（可选）
     * @return 载荷的JSON字符串
     */
    static std::string buildPayload(const std::map<std::string, std::string>& customClaims,
                                    long long issuedAt,
                                    std::optional<long long> expiresAt);
    
    /**
     * @brief 转义JSON字符串
     * @brief Escape JSON string
     * @param input 输入字符串
     * @return 转义后的字符串
     */
    static std::string escapeJson(const std::string& input);

    /**
     * @brief 分割JWT令牌
     * @brief Split JWT token
     * @param token JWT令牌
     * @return 分割后的部分（头部、载荷、签名）
     */
    static std::vector<std::string> splitToken(const std::string& token);
    
    /**
     * @brief 从载荷中提取数值类型声明
     * @brief Extract numeric claim from payload
     * @param payload 载荷JSON字符串
     * @param claimKey 声明键名
     * @return 数值声明值，不存在则返回std::nullopt
     */
    static std::optional<long long> extractNumericClaim(const std::string& payload, const std::string& claimKey);

    /**
     * @brief 解析TTL时间规格
     * @brief Parse TTL time specification
     * @param ttlSpec TTL时间规格字符串
     * @return 转换后的秒数
     */
    static long long parseTTL(const std::string& ttlSpec);
    
    /**
     * @brief 解析JSON对象
     * @brief Parse JSON object
     * @param json JSON字符串
     * @return 解析后的键值对映射
     */
    static std::map<std::string, std::string> parseJsonObject(const std::string& json);
    
    /**
     * @brief 反转义JSON字符串
     * @brief Unescape JSON string
     * @param input 转义后的JSON字符串
     * @return 反转义后的原始字符串
     */
    static std::string unescapeJson(const std::string& input);

    /**
     * @brief 原生SHA-256实现（返回原始字节）
     * @brief Native SHA-256 implementation (returns raw bytes)
     * @param data 要哈希的数据字节数组
     * @return 哈希后的字节数组
     */
    static std::vector<uint8_t> sha256_bytes(const std::vector<uint8_t>& data) ;

    /**
     * @brief HMAC-SHA256实现（基于sha256_bytes）
     * @brief HMAC-SHA256 implementation (based on sha256_bytes)
     * @param key 密钥字节数组
     * @param data 要哈希的数据字节数组
     * @return HMAC哈希后的字节数组
     */
    static std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& key,const std::vector<uint8_t>& data) ;

    /**
     * @brief 生成随机盐值（16字节，足够安全）
     * @brief Generate random salt (16 bytes, secure enough)
     * @return 随机生成的盐值
     */
    static std::string generate_salt() ;

    /**
     * @brief 字节转十六进制字符串（便于存储）
     * @brief Convert bytes to hexadecimal string (for storage)
     * @param bytes 字节字符串
     * @return 十六进制表示的字符串
     */
    static std::string bytes_to_hex(const std::string & bytes);

    /**
     * @brief 十六进制字符串转字节数组（解析盐值和哈希时需要）
     * @brief Convert hexadecimal string to bytes array (needed for parsing salt and hash)
     * @param hex 十六进制字符串
     * @return 对应的字节数组
     */
    static std::string hex_to_bytes(const std::string& hex) ;

    /**
     * @brief 常量时间比较（防时序攻击）：无论内容是否相同，比较时间一致
     * @brief Constant time comparison (prevent timing attacks): comparison time is consistent regardless of content
     * @param a 第一个字符串
     * @param b 第二个字符串
     * @return 两个字符串相等返回true，否则返回false
     */
    static bool constant_time_compare(const std::string & a, const std::string & b) ;

    std::string secret_;         ///< JWT签名密钥 (JWT signing secret key)
    Algorithm algorithm_;        ///< 使用的加密算法 (Encryption algorithm used)
    long long defaultTTL_ = 60 * 24 * 7; ///< 默认令牌过期时间（秒），默认一周 (Default token expiration time in seconds, default one week)
    static JWT* instance_;       ///< JWT单例实例指针 (JWT singleton instance pointer)

    /**
     * @brief 从JsonValue获取字符串值
     * @brief Get string value from JsonValue
     * @param value JsonValue对象
     * @param key 键名
     * @return 对应的字符串值
     */
    static std::string getStringFromJsonValue(const JsonValue &value, const std::string &key);
};

#endif //CBSF_JWT_H
