/**
 * @file jwt.cpp
 * @brief JWT (JSON Web Token) 功能实现
 * 
 * 此文件实现了JWT的核心功能，包括令牌生成、验证、签名和解析等操作。
 * 实现采用HS256算法进行签名，支持Base64URL编码/解码，以及HMAC-SHA256加密。
 * 
 * This file implements the core functionality of JWT, including token generation,
 * verification, signing, and parsing operations. It uses the HS256 algorithm for
 * signing, supports Base64URL encoding/decoding, and HMAC-SHA256 encryption.
 */

#include "../include/jwt.h"
#include "JsonValue.h"
//#include "../include/config.h"

#include <array>
#include <cstring>
#include <iomanip>
#include <mutex>

namespace {

/**
 * @brief SHA256算法常量数组
 * 
 * 这些是SHA256哈希算法中使用的常量值，基于前64个质数的立方根的小数部分的前32位。
 * These are the constant values used in the SHA256 hash algorithm, based on 
 * the first 32 bits of the fractional parts of the cube roots of the first 64 primes.
 */
constexpr std::array<std::uint32_t, 64> kSha256Constants{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};



} // namespace

/**
 * @brief JWT类的单例实例指针
 * 
 * 静态成员变量，用于存储JWT类的唯一实例，实现单例设计模式。
 * Static member variable to store the unique instance of the JWT class, implementing the singleton design pattern.
 */
JWT* JWT::instance_ = nullptr;

/**
 * @brief JWT类的构造函数，初始化JWT实例
 *
 * 该构造函数用于创建JWT对象，设置默认算法为HS256，默认生存时间(TTL)为3600秒。
 * 它会检查是否已经存在JWT实例，如果存在则输出错误信息。
 *
 * This constructor creates a JWT object, sets the default algorithm to HS256, 
 * and default time-to-live (TTL) to 3600 seconds. It checks if a JWT instance 
 * already exists and outputs an error message if it does.
 *
 * @param expiresIn JWT令牌过期时间的字符串表示
 * @param jwtPrivateKeyPath JWT私钥文件路径
 * @note 这是一个单例模式的实现，确保系统中只有一个JWT实例
 * @note This is a singleton pattern implementation, ensuring only one JWT instance exists in the system
 */
JWT::JWT(const std::string& expiresIn, const std::string& jwtPrivateKeyPath) : algorithm_(Algorithm::HS256), defaultTTL_(3600) {
    // 检查是否已存在JWT实例，如果存在则输出错误信息
    // Check if JWT instance already exists, output error message if it does
    if (instance_ != nullptr) {
        std::cerr<<"JWT instance already exists" << std::endl;
        return;
    }
    if (!expiresIn.empty()) {
        auto ttl = parseTTL(expiresIn);
        if (ttl > 0) {
            defaultTTL_ = ttl;
        }
    }

    if (!jwtPrivateKeyPath.empty()) {
        loadSecretFromFile(jwtPrivateKeyPath);
    }
    instance_ = this;
}

/**
 * @brief JWT类的构造函数，使用指定的密钥和过期时间
 * 
 * 创建JWT对象，设置密钥、算法为HS256，并指定默认的令牌生存时间。
 * Creates a JWT object, sets the secret key, algorithm to HS256, and specifies the default token time-to-live.
 * 
 * @param secret JWT签名和验证使用的密钥
 * @param ttlSeconds 默认令牌生存时间（秒），如果小于等于0则使用3600秒
 * @throws std::invalid_argument 当提供的密钥为空时抛出异常
 */
JWT::JWT(std::string secret, long long ttlSeconds)
        : secret_(std::move(secret)), algorithm_(Algorithm::HS256), defaultTTL_(ttlSeconds > 0 ? ttlSeconds : 3600) {
    if (secret_.empty()) {
        throw std::invalid_argument("JWT secret must not be empty");
    }
}

/**
 * @brief 设置JWT的密钥
 * 
 * 设置用于JWT令牌签名和验证的密钥。密钥不能为空，否则会抛出异常。
 * Sets the secret key used for JWT token signing and verification. The secret cannot be empty.
 * 
 * @param secret 要设置的密钥字符串
 * @throws std::invalid_argument 当传入的密钥为空时抛出异常
 * @throws std::invalid_argument Throws an exception when the provided secret is empty
 */
void JWT::setSecret(const std::string& secret) {
    // 检查密钥是否为空
    if (secret.empty()) {
        // 如果密钥为空，抛出无效参数异常
        throw std::invalid_argument("JWT secret must not be empty");
    }
    // 将传入的密钥保存到成员变量中
    secret_ = secret;
}

/**
 * @brief 循环右移函数（Rotor函数）
 * 
 * 将一个32位无符号整数循环右指定位数，SHA256算法的核心操作之一。
 * Performs a cyclic right shift on a 32-bit unsigned integer, one of the core operations of the SHA256 algorithm.
 * 
 * @param value 要进行循环右移的32位无符号整数值
 * @param count 循环右移的位数，范围应在0到31之间
 * @return 返回循环右移后的32位无符号整数值
 *
 * 该函数通过将值右移指定位数，并将剩余高位部分左移相应位数，
 * 然后通过按位或操作组合这两个部分，实现循环右移效果。
 * 例如：rotor(0b1100, 2) 将返回 0b0011
 * 
 * This function implements cyclic right shift by shifting the value right by the specified number of bits,
 * shifting the remaining high bits left by the corresponding number of bits, and combining the two parts
 * with a bitwise OR operation. For example: rotor(0b1100, 2) will return 0b0011
 */
inline std::uint32_t rotor(std::uint32_t value, std::uint32_t count) {
    // 右移count位，并将左移(32-count)位后的结果进行按位或操作
    // 实现循环右移效果
    return (value >> count) | (value << (32 - count));
}
/**
 * @brief 从文件中加载密钥
 * 
 * 从指定的文件路径读取JWT密钥。文件以二进制模式打开，以确保正确读取所有字符。
 * Loads the JWT secret key from the specified file path. The file is opened in binary mode to ensure all characters are read correctly.
 * 
 * @param path 密钥文件的路径
 * @throws std::runtime_error 当文件无法打开时抛出异常
 * @throws std::runtime_error Throws a runtime exception if the file cannot be opened
 */
void JWT::loadSecretFromFile(const std::string& path) {
    // 以二进制模式打开文件
    // Open file in binary mode
    std::ifstream file(path, std::ios::binary);

    // 检查文件是否成功打开
    // Check if file opened successfully
    if (!file.is_open()) {
        // 如果文件打开失败，抛出运行时异常
        // Throw runtime exception if file opening fails
        throw std::runtime_error("Failed to open secret file: " + path);
    }

    // 使用字符串流来读取文件内容
    // Use string stream to read file content
    std::ostringstream oss;
    // 将文件内容读取到字符串流中
    // Read file content into string stream
    oss << file.rdbuf();
    // 设置读取到的内容为密钥
    // Set the read content as the secret key
//    std::cout<<oss.str()<<std::endl;
    setSecret(oss.str());
}

/**
 * @brief 设置默认令牌生存时间
 * 
 * 设置JWT令牌的默认生存时间（TTL），单位为秒。TTL必须为正数。
 * Sets the default time-to-live (TTL) for JWT tokens in seconds. TTL must be a positive value.
 * 
 * @param ttlSeconds 默认令牌生存时间（秒）
 * @throws std::invalid_argument 当TTL小于等于0时抛出异常
 */
void JWT::setDefaultTTL(long long ttlSeconds) {
    if (ttlSeconds <= 0) {
        throw std::invalid_argument("TTL must be positive");
    }
    defaultTTL_ = ttlSeconds;
}

/**
 * @brief 生成JWT令牌
 * 
 * 创建并返回一个完整的JWT令牌字符串，包含头部、载荷和签名部分。
 * Generates and returns a complete JWT token string, including header, payload, and signature parts.
 * 
 * @param customClaims 自定义声明，一个键值对映射
 * @param ttlSeconds 令牌有效期（秒），-1表示使用默认有效期
 * @return 生成的JWT令牌字符串
 * @throws std::runtime_error 当密钥未设置时抛出异常
 * @throws std::runtime_error Throws a runtime exception if the secret key is not set
 */
std::string JWT::generateToken(const std::map<std::string, std::string>& customClaims, long long ttlSeconds) const {
    // 检查密钥是否已设置
    // Check if secret key is set
    if (secret_.empty()) {
//        std::cout<<"empty"<<std::endl;
        throw std::runtime_error("JWT secret is not set");
    }

    // 构建并编码JWT头部
    // Build and encode JWT header
    const auto header = buildHeader();
    const auto headerEncoded = base64UrlEncode(header);

    // 获取当前时间戳（秒）
    // Get current timestamp in seconds
    auto now = std::chrono::system_clock::now();
    auto issuedAt = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // 处理令牌有效期
    // Handle token expiration time
    long long effectiveTTL = ttlSeconds;
    if (effectiveTTL < 0) {
        effectiveTTL = defaultTTL_;  // 使用默认有效期
        // Use default TTL if negative value provided
    }
    std::optional<long long> expiresAt;
    if (effectiveTTL > 0) {
        expiresAt = issuedAt + effectiveTTL;  // 计算过期时间戳
        // Calculate expiration timestamp
    }

    // 构建并编码JWT载荷
    // Build and encode JWT payload
    const auto payload = buildPayload(customClaims, issuedAt, expiresAt);
    const auto payloadEncoded = base64UrlEncode(payload);

    // 创建签名输入并生成签名
    // Create signing input and generate signature
    const std::string signingInput = headerEncoded + "." + payloadEncoded;
    const auto signature = base64UrlEncode(hmacSha256(signingInput, secret_));

    // 组合头部、载荷和签名，返回完整的JWT令牌
    // Combine header, payload and signature to return complete JWT token
    return signingInput + "." + signature;
}

/**
 * @brief 验证JWT令牌的有效性
 * 
 * 验证JWT令牌的签名是否正确，并检查令牌是否已过期。
 * Verifies if the JWT token signature is correct and checks if the token has expired.
 * 
 * @param token 待验证的JWT令牌字符串
 * @param payloadJson 用于存储载荷JSON字符串的输出参数，可为nullptr
 * @return 验证成功返回true，失败返回false
 * @return Returns true if the token is valid, false otherwise
 */
bool JWT::verifyToken(const std::string& token, std::string* payloadJson) const {
    // 检查密钥是否为空
    // Check if secret key is empty
    if (secret_.empty()) {
        return false;
    }

    // 分割令牌为三部分（头部、载荷、签名）
    // Split token into three parts (header, payload, signature)
    const auto parts = splitToken(token);
    // 检查令牌是否由三部分组成
    // Check if token consists of three parts
    if (parts.size() != 3) {
        return false;
    }

    // 构造待签名的字符串（头部+载荷）
    // Construct the signing input string (header + payload)
    const std::string signingInput = parts[0] + "." + parts[1];
    // 计算期望的签名
    // Calculate expected signature
    const std::string expectedSignature = base64UrlEncode(hmacSha256(signingInput, secret_));
    // 验证签名是否匹配
    // Verify if signature matches
    if (expectedSignature != parts[2]) {
        return false;
    }

    // 解码载荷部分
    // Decode payload part
    const std::string payload = base64UrlDecode(parts[1]);

    // 检查过期时间(exp)声明
    // Check expiration time (exp) claim
    const auto expClaim = extractNumericClaim(payload, "exp");
    if (expClaim.has_value()) {
        // 获取当前时间戳
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto current = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        // 检查是否过期
        // Check if token has expired
        if (current > expClaim.value()) {
            return false;
        }
    }

    // 如果提供了payloadJson参数，则将载荷JSON字符串存入
    // If payloadJson parameter is provided, store payload JSON string
    if (payloadJson != nullptr) {
        *payloadJson = payload;
    }
    return true;
}

/**
 * @brief 解析JWT令牌中的声明（claims）部分
 * 
 * 验证令牌有效性并解析其中的声明部分为键值对映射。
 * Verifies token validity and parses its claims into a key-value map.
 * 
 * @param token 要解析的JWT令牌字符串
 * @return 返回一个包含字符串映射的std::optional对象，如果解析失败则返回std::nullopt
 * @return Returns a std::optional containing a string map, or std::nullopt if parsing fails
 */
std::optional<std::map<std::string, std::string>> JWT::parseClaims(const std::string& token) const {
    std::string payload; // 用于存储令牌的payload部分
    if (!verifyToken(token, &payload)) { // 验证令牌有效性，如果验证失败
        return std::nullopt; // 返回空值表示解析失败
    }
    try {
        return parseJsonObject(payload); // 尝试解析payload为JSON对象
    } catch (const std::exception&) { // 捕获解析过程中可能出现的异常
        return std::nullopt; // 如果解析失败，返回空值
    }
}

/**
 * @brief 获取JWT使用的算法
 * 
 * 返回当前JWT实例使用的签名算法。
 * Returns the signature algorithm used by the current JWT instance.
 * 
 * @return JWT::Algorithm 枚举类型，表示使用的算法
 */
JWT::Algorithm JWT::algorithm() const {
    return algorithm_;
}

/**
 * @brief 构建JWT头部信息
 * 
 * 生成标准的JWT头部JSON字符串，指定使用HS256算法和JWT令牌类型。
 * Generates a standard JWT header JSON string, specifying the HS256 algorithm and JWT token type.
 * 
 * @return 返回包含算法和类型的标准JWT头部字符串
 * @return Returns a standard JWT header string containing algorithm and type
 *
 * 该函数返回一个固定的JWT头部字符串，使用HS256算法作为签名算法，
 * 并指定令牌类型为JWT(JSON Web Token)
 * 
 * This function returns a fixed JWT header string, using HS256 as the signature algorithm,
 * and specifying the token type as JWT (JSON Web Token)
 */
std::string JWT::buildHeader() {
    // 使用静态常量存储标准JWT头部字符串
    // 内容为JSON格式，包含算法(alg)和类型(typ)字段
    static const std::string header = R"({"alg":"HS256","typ":"JWT"})";
    // 返回预定义的JWT头部
    return header;
}

/**
 * @brief JWT类的base64UrlEncode方法
 * 
 * 实现符合JWT规范的Base64URL编码，确保URL安全。
 * Implements Base64URL encoding according to the JWT specification to ensure URL safety.
 * 
 * @param data 需要进行base64Url编码的原始数据
 * @return 返回编码后的字符串
 * 
 * 该方法实现了base64Url编码，它是一种base64编码的变体，主要用于URL安全。
 * 与标准base64编码相比，它将'+'替换为'-'，'/'替换为'_'，并移除了末尾的'='填充字符。
 * 
 * This method implements base64Url encoding, which is a variant of base64 encoding primarily used for URL safety.
 * Compared to standard base64 encoding, it replaces '+' with '-', '/' with '_', and removes trailing '=' padding characters.
 */
std::string JWT::base64UrlEncode(const std::string& data) {
    // 定义base64编码表
    static const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    // 预分配足够的内存空间，提高性能
    encoded.reserve(((data.size() + 2) / 3) * 4);

    std::size_t i = 0;
    // 处理完整的三字节块
    while (i + 3 <= data.size()) {
        auto b1 = static_cast<unsigned char>(data[i]);
        auto b2 = static_cast<unsigned char>(data[i + 1]);
        auto b3 = static_cast<unsigned char>(data[i + 2]);

        // 将三个字节转换为四个base64字符
        encoded.push_back(base64Chars[b1 >> 2]);
        encoded.push_back(base64Chars[((b1 & 0x03) << 4) | (b2 >> 4)]);
        encoded.push_back(base64Chars[((b2 & 0x0F) << 2) | (b3 >> 6)]);
        encoded.push_back(base64Chars[b3 & 0x3F]);
        i += 3;
    }

    // 处理剩余的不完整字节块
    if (i < data.size()) {
        auto b1 = static_cast<unsigned char>(data[i]);
        encoded.push_back(base64Chars[b1 >> 2]);
        if (i + 1 < data.size()) {
            auto b2 = static_cast<unsigned char>(data[i + 1]);
            encoded.push_back(base64Chars[((b1 & 0x03) << 4) | (b2 >> 4)]);
            encoded.push_back(base64Chars[(b2 & 0x0F) << 2]);
            encoded.push_back('=');
        } else {
            encoded.push_back(base64Chars[(b1 & 0x03) << 4]);
            encoded.push_back('=');
            encoded.push_back('=');
        }
    }

    // 将base64编码转换为URL安全的格式
    for (char& ch : encoded) {
        if (ch == '+') {
            ch = '-';
        } else if (ch == '/') {
            ch = '_';
        }
    }

    // 移除末尾的填充字符'='
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }

    return encoded;
}

/**
 * @brief Base64URL解码函数
 * 
 * 将Base64URL编码的字符串解码为原始字符串。
 * Decodes a Base64URL encoded string back to its original form.
 * 
 * @param data 需要解码的Base64URL字符串
 * @return 解码后的原始字符串
 * 
 * 该函数将Base64URL编码的字符串转换为原始字符串。Base64URL是Base64的变种，
 * 使用'-'和'_'替代了标准的'+'和'/'，同时不使用填充字符'='。
 * 
 * This function converts a Base64URL encoded string to its original string. Base64URL is a variant of Base64,
 * using '-' and '_' instead of the standard '+' and '/', and not using padding character '='.
 */
std::string JWT::base64UrlDecode(const std::string& data) {
    std::string base64 = data;
    // 将Base64URL的特殊字符转换为标准Base64字符
    for (char& ch : base64) {
        if (ch == '-') {
            ch = '+';
        } else if (ch == '_') {
            ch = '/';
        }
    }
    // 添加填充字符'='，使字符串长度为4的倍数
    while (base64.size() % 4 != 0) {
        base64.push_back('=');
    }

    // Lambda函数：将Base64字符转换为对应的6位整数值
    auto decodeChar = [](char ch) -> int {
        if ('A' <= ch && ch <= 'Z') return ch - 'A';
        if ('a' <= ch && ch <= 'z') return ch - 'a' + 26;
        if ('0' <= ch && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        return -1;
    };

    std::string decoded;
    // 预分配内存，提高性能
    decoded.reserve((base64.size() / 4) * 3);

    // 每次处理4个Base64字符
    for (std::size_t i = 0; i < base64.size(); i += 4) {
        int val1 = decodeChar(base64[i]);
        int val2 = decodeChar(base64[i + 1]);
        int val3 = base64[i + 2] == '=' ? -1 : decodeChar(base64[i + 2]);
        int val4 = base64[i + 3] == '=' ? -1 : decodeChar(base64[i + 3]);

        // 将4个6位值转换为3个8位字节
        decoded.push_back(static_cast<char>((val1 << 2) | (val2 >> 4)));
        if (val3 >= 0) {
            decoded.push_back(static_cast<char>(((val2 & 0x0F) << 4) | (val3 >> 2)));
            if (val4 >= 0) {
                decoded.push_back(static_cast<char>(((val3 & 0x03) << 6) | val4));
            }
        }
    }

    return decoded;
}

/**
 * @brief 计算输入字符串的SHA256哈希值
 * 
 * 实现SHA256加密算法，计算输入数据的哈希值。
 * Implements the SHA256 encryption algorithm to compute the hash value of input data.
 * 
 * @param data 输入字符串
 * @return 返回计算得到的SHA256哈希值字符串
 * @return Returns the computed SHA256 hash value as a string
 */
std::string JWT::sha256(const std::string& data) {
    // 初始化SHA256哈希值的初始常量
    std::array<std::uint32_t, 8> hash{
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // 将输入字符串转换为字节数组
    std::vector<std::uint8_t> message(data.begin(), data.end());
    // 记录原始数据的比特长度
    const std::uint64_t originalBitLength = static_cast<std::uint64_t>(message.size()) * 8;

    // 添加填充位1
    message.push_back(0x80);
    // 添加填充位0，直到消息长度满足模64等于56
    while ((message.size() % 64) != 56) {
        message.push_back(0x00);
    }

    // 添加原始消息长度信息（64位大端序）
    for (int i = 7; i >= 0; --i) {
        message.push_back(static_cast<std::uint8_t>((originalBitLength >> (i * 8)) & 0xFF));
    }

    // 处理消息的每个512位（64字节）的块
    for (std::size_t offset = 0; offset < message.size(); offset += 64) {
        std::uint32_t w[64];

        // 将当前64字节的块分解为16个32位字
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(message[offset + 4 * i]) << 24) |
                   (static_cast<std::uint32_t>(message[offset + 4 * i + 1]) << 16) |
                   (static_cast<std::uint32_t>(message[offset + 4 * i + 2]) << 8) |
                   (static_cast<std::uint32_t>(message[offset + 4 * i + 3]));
        }

        // 扩展16个字为64个字
        for (int i = 16; i < 64; ++i) {
            std::uint32_t s0 = rotor(w[i - 15], 7) ^ rotor(w[i - 15], 18) ^ (w[i - 15] >> 3);
            std::uint32_t s1 = rotor(w[i - 2], 17) ^ rotor(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        // 初始化哈希值的工作变量
        std::uint32_t a = hash[0];
        std::uint32_t b = hash[1];
        std::uint32_t c = hash[2];
        std::uint32_t d = hash[3];
        std::uint32_t e = hash[4];
        std::uint32_t f = hash[5];
        std::uint32_t g = hash[6];
        std::uint32_t h = hash[7];

        // 主压缩函数循环
        for (int i = 0; i < 64; ++i) {
            std::uint32_t s1 = rotor(e, 6) ^ rotor(e, 11) ^ rotor(e, 25);
            std::uint32_t ch = (e & f) ^ ((~e) & g);
            std::uint32_t temp1 = h + s1 + ch + kSha256Constants[i] + w[i];
            std::uint32_t s0 = rotor(a, 2) ^ rotor(a, 13) ^ rotor(a, 22);
            std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            std::uint32_t temp2 = s0 + maj;

            // 更新工作变量
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        // 更新哈希值
        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    // 将最终的哈希值转换为32字节的字符串
    std::string result;
    result.reserve(32);
    for (std::uint32_t part : hash) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            result.push_back(static_cast<char>((part >> shift) & 0xFF));
        }
    }
    return result;
}

/**
 * @brief HMAC-SHA256 签名算法实现
 * 
 * HMAC (Hash-based Message Authentication Code) 是一种基于哈希的消息认证码算法，
 * 此函数使用SHA-256作为哈希函数实现HMAC签名，用于JWT令牌的安全验证。
 * 
 * HMAC (Hash-based Message Authentication Code) is a hash-based message authentication code algorithm.
 * This function implements HMAC signing using SHA-256 as the hash function, used for secure JWT token verification.
 * 
 * @param data 要签名的原始数据
 * @param key 用于签名的密钥
 * @return 返回HMAC-SHA256签名结果
 * @return Returns the HMAC-SHA256 signature result
 */
std::string JWT::hmacSha256(const std::string& data, const std::string& key) {
    constexpr std::size_t blockSize = 64;
    std::string actualKey = key;

    // 处理密钥
    if (actualKey.size() > blockSize) {
        actualKey = sha256(actualKey);
    }
    if (actualKey.size() < blockSize) {
        actualKey.append(blockSize - actualKey.size(), '\0');
    }

    // 创建填充密钥
    std::string oKeyPad(blockSize, '\0');
    std::string iKeyPad(blockSize, '\0');
    for (std::size_t i = 0; i < blockSize; ++i) {
        oKeyPad[i] = static_cast<char>(actualKey[i] ^ 0x5c);
        iKeyPad[i] = static_cast<char>(actualKey[i] ^ 0x36);
    }

    // 计算内部哈希：iKeyPad + data
    std::string innerData = iKeyPad + data;
    std::string innerHash = sha256(innerData);

    // 计算最终哈希：oKeyPad + innerHash
    std::string outerData = oKeyPad + innerHash;
    return sha256(outerData);
}

/**
 * 对输入的字符串进行JSON转义处理
 * @param input 需要转义的原始字符串
 * @return 转义后的字符串，确保可以安全地在JSON格式中使用
 */
std::string JWT::escapeJson(const std::string& input) {
    std::string escaped;  // 用于存储转义后的字符串
    // 预分配内存，优化性能，假设转义后字符串长度最多增加4个字符
    escaped.reserve(input.size() + 4);
    // 遍历输入字符串的每个字符
    for (char ch : input) {
        switch (ch) {
            // 处理需要转义的特殊字符
            case '\\': escaped += "\\\\"; break;  // 反斜杠转义为两个反斜杠
            case '"': escaped += "\\\""; break;  // 双引号转义为反斜杠加双引号
            case '\b': escaped += "\\b"; break;  // 退格符转义
            case '\f': escaped += "\\f"; break;  // 换页符转义
            case '\n': escaped += "\\n"; break;  // 换行符转义
            case '\r': escaped += "\\r"; break;  // 回车符转义
            case '\t': escaped += "\\t"; break;  // 制表符转义
            default:
                // 处理控制字符（ASCII码小于0x20的字符）
                if (static_cast<unsigned char>(ch) < 0x20) {
                    std::ostringstream oss;
                    // 将控制字符转换为Unicode转义序列格式（\uXXXX）
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (static_cast<int>(ch) & 0xFF);
                    escaped += oss.str();
                } else {
                    // 其他字符直接添加到结果中
                    escaped += ch;
                }
                break;
        }
    }
    return escaped;  // 返回转义后的字符串
}

/**
 * @brief 构建JWT的payload部分（使用JsonValue优化）
 * 先构造JsonValue对象，再序列化为JSON字符串，避免手动拼接错误
 */
std::string JWT::buildPayload(const std::map<std::string, std::string>& customClaims,
                              long long issuedAt,
                              std::optional<long long> expiresAt) {
    // 第一步：构造JsonValue对象（JSON对象类型）
    std::map<std::string, JsonValue> payloadMap;

    // 添加标准声明：iat（签发时间）
    payloadMap.emplace("iat", JsonValue(static_cast<double>(issuedAt)));

    // 添加可选标准声明：exp（过期时间）
    if (expiresAt.has_value()) {
        payloadMap.emplace("exp", JsonValue(static_cast<double>(expiresAt.value())));
    }

    // 添加自定义声明（自动处理字符串转义）
    for (const auto& [key, value] : customClaims) {
        // 直接使用JsonValue的字符串构造，转义逻辑由JsonValue::escapeJsonString自动处理
        payloadMap.emplace(key, JsonValue(value));
    }

    // 第二步：使用JsonValue序列化为标准JSON字符串
    JsonValue payloadValue(payloadMap);
    return payloadValue.toJson();
}

/**
 * @brief 将JWT令牌字符串按照点号(.)分割成多个部分
 * @param token 完整的JWT令牌字符串
 * @return std::vector<std::string> 分割后的令牌部分数组
 */
std::vector<std::string> JWT::splitToken(const std::string& token) {
    std::vector<std::string> parts;  // 用于存储分割后的令牌部分
    std::size_t start = 0;  // 当前查找的起始位置
    // 遍历令牌字符串，查找点号(.)作为分割点
    while (start <= token.size()) {
        auto pos = token.find('.', start);  // 查找下一个点号的位置
        // 如果找不到点号，将剩余部分添加到结果中并结束循环
        if (pos == std::string::npos) {
            parts.emplace_back(token.substr(start));
            break;
        }
        // 添加当前部分到结果中
        parts.emplace_back(token.substr(start, pos - start));
        start = pos + 1;  // 更新查找起始位置为点号之后的位置
    }
    return parts;  // 返回分割后的令牌部分数组
}

/**
 * 从JWT负载中提取指定的数字声明
 * @param payload JWT的负载字符串
 * @param claimKey 要提取的声明键名
 * @return 如果找到有效的数字声明，返回其值(std::optional<long long>)；否则返回std::nullopt
 */
std::optional<long long> JWT::extractNumericClaim(const std::string& payload, const std::string& claimKey) {
    // 构建要查找的声明键模式，格式为"\"claimKey\":"
    const std::string pattern = "\"" + claimKey + "\":";
    // 在负载中查找声明键的位置
    auto pos = payload.find(pattern);
    // 如果未找到声明键，返回空值
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    // 移动到声明值的起始位置
    pos += pattern.size();
    // 跳过可能的空白字符
    while (pos < payload.size() && std::isspace(static_cast<unsigned char>(payload[pos]))) {
        ++pos;
    }
    // 检查是否为负数
    bool negative = false;
    if (pos < payload.size() && payload[pos] == '-') {
        negative = true;
        ++pos;
    }
    // 解析数字值
    long long value = 0;
    bool hasDigit = false;
    // 遍历数字字符并构建数值
    while (pos < payload.size() && std::isdigit(static_cast<unsigned char>(payload[pos]))) {
        hasDigit = true;
        value = value * 10 + (payload[pos] - '0');
        ++pos;
    }
    // 如果没有找到数字字符，返回空值
    if (!hasDigit) {
        return std::nullopt;
    }
    // 返回解析后的数值，考虑正负号
    return negative ? -value : value;
}

/**
 * 解析TTL（Time To Live）时间字符串，将其转换为秒数
 * @param ttlSpec TTL时间字符串，例如"1h30m"、"2d"等
 * @return 转换后的总秒数，如果解析失败则返回0
 */
long long JWT::parseTTL(const std::string& ttlSpec) {
    // 如果输入字符串为空，直接返回0
    if (ttlSpec.empty()) {
        return 0;
    }

    long long total = 0;  // 存储转换后的总秒数
    std::size_t i = 0;    // 当前处理位置索引
    // 遍历整个字符串
    while (i < ttlSpec.size()) {
        // 跳过空白字符
        while (i < ttlSpec.size() && std::isspace(static_cast<unsigned char>(ttlSpec[i]))) {
            ++i;
        }
        // 如果已经到达字符串末尾，退出循环
        if (i >= ttlSpec.size()) {
            break;
        }
        long long value = 0;  // 存储数值部分
        bool hasDigit = false; // 标记是否遇到数字
        // 解析数字部分
        while (i < ttlSpec.size() && std::isdigit(static_cast<unsigned char>(ttlSpec[i]))) {
            hasDigit = true;
            value = value * 10 + (ttlSpec[i] - '0');  // 将字符转换为数字
            ++i;
        }
        // 如果没有遇到有效数字，返回0表示解析失败
        if (!hasDigit) {
            return 0;
        }
        // 如果到达字符串末尾，将数值累加到总数中并退出
        if (i >= ttlSpec.size()) {
            total += value;
            break;
        }
        // 获取单位字符并转换为小写
        char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(ttlSpec[i])));
        ++i;
        long long multiplier = 0;  // 时间单位对应的秒数乘数
        switch (unit) {
            case 's': multiplier = 1; break;    // 秒
            case 'm': multiplier = 60; break;   // 分钟
            case 'h': multiplier = 3600; break; // 小时
            case 'd': multiplier = 86400; break; // 天
            default:
                return 0;  // 遇到未知单位，返回0表示解析失败
        }
        // 累加转换后的秒数
        total += value * multiplier;
    }
    return total;
}

/**
 * @brief 解析JSON对象字符串为键值对映射（使用JsonValue优化）
 * 利用JsonValue的完整解析能力，自动处理转义字符、嵌套检查、语法验证
 */
std::map<std::string, std::string> JWT::parseJsonObject(const std::string& json) {
    std::map<std::string, std::string> result;
    JsonValue jsonValue;

    try {
        jsonValue.fromJson(json);
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON parse failed: " + std::string(e.what()));
    }

    // 第二步：检查解析结果是否为JSON对象类型
    if (jsonValue.getType() != JsonValue::OBJECT) {
        throw std::runtime_error("Invalid JSON: not an object");
    }

    // 第三步：提取对象中的所有键值对，转换为string-string映射
    auto jsonObject = jsonValue.asObject();
    for (const auto& [key, value] : jsonObject) {
        // 提取值并确保为string类型（非string类型会自动转为对应字符串形式）
        switch (value.getType()) {
            case JsonValue::STRING:
                result.emplace(key, value.asString());
                break;
            case JsonValue::NUMBER:
                // 数字类型转为string（保持原始数值格式）
                result.emplace(key, value.toJson());
                break;
            case JsonValue::BOOLEAN:
                // 布尔类型转为"true"/"false"字符串
                result.emplace(key, value.toJson());
                break;
            case JsonValue::NULL_TYPE:
                // null类型转为"null"字符串
                result.emplace(key, "null");
                break;
            default:
                // 不支持对象/数组作为值（如需支持可扩展）
                throw std::runtime_error("Unsupported value type for key: " + key);
        }
    }

    return result;
}


/**
 * @brief JSON字符串反转义（复用JsonValue的parseString逻辑）
 */
std::string JWT::unescapeJson(const std::string& input) {
    // 构造完整的JSON字符串（包裹双引号，符合parseString的输入要求）
    std::string jsonString = "\"" + input + "\"";
    size_t pos = 0;

    try {
        return JsonValue::parseString(jsonString, pos);
    } catch (const std::exception& e) {
        throw std::runtime_error("Unescape JSON failed: " + std::string(e.what()));
    }
}

/**
 * @brief 辅助函数：从JsonValue提取string值（含类型检查）
 */
std::string JWT::getStringFromJsonValue(const JsonValue& value, const std::string& key) {
    if (value.getType() != JsonValue::STRING) {
        throw std::runtime_error("Key '" + key + "' is not a string type");
    }
    return value.asString();
}

/**
     * 安全的密码加密：使用PBKDF2-HMAC-SHA256
     * @param password 明文密码
     * @return 存储格式：salt(hex):iterations:hash(hex)
     */
std::string JWT::encrypt_password(const std::string& password) {
    const size_t iterations = 100000; // 迭代次数（可根据性能调整）
    std::string salt = generate_salt();

    // PBKDF2核心：多次迭代HMAC-SHA256
    std::string dk(32, 0x00); // 输出32字节哈希
    std::string U, T(32, 0);
    for (size_t i = 1; i <= 1; ++i) { // 只需要1个块（32字节）
        // 盐值 + 4字节大端序号（i=1）
        std::string salt_i = salt;
        salt_i.push_back(static_cast<char>((i >> 24) & 0xFF));
        salt_i.push_back(static_cast<char>((i >> 16) & 0xFF));
        salt_i.push_back(static_cast<char>((i >> 8) & 0xFF));
        salt_i.push_back(static_cast<char>(i & 0xFF));

        U = hmacSha256(password, salt_i);
        T = U;
        for (size_t j = 1; j < iterations; ++j) {
            U = hmacSha256(password, U);
            for (size_t k = 0; k < 32; ++k) {
                T[k] ^= U[k]; // 异或累积
            }
        }
        dk = T;
    }

    return bytes_to_hex(salt) + ":" + std::to_string(iterations) + ":" + bytes_to_hex(dk);
}

/**
 * @brief 生成加密安全的盐值
 * 生成 16 字节（128 位）加密安全随机数，编码为 32 字符十六进制字符串
 * 满足密码学安全要求，跨平台兼容，生成的盐值可直接存储/传输
 */
std::string JWT::generate_salt() {
    const size_t salt_length = 16; // 16 字节 = 128 位，足够密码学安全
    std::string salt_bytes(salt_length, 0x00);

    try {
        // 1. 初始化加密安全随机数生成器
        std::random_device rd;
        // 双重种子：random_device + 当前时间戳，增强随机性（应对 rd 伪随机的情况）
        uint64_t seed = rd() ^ static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::mt19937_64 gen(seed); // 64 位 Mersenne Twister 生成器，适合加密场景
        std::uniform_int_distribution<uint8_t> dist(0x00, 0xFF); // 生成 0-255 的均匀分布随机字节

        // 2. 生成 16 字节随机数据
        for (auto& byte : salt_bytes) {
            byte = dist(gen);
        }

        // 3. 转为十六进制字符串（可打印、易存储）
        return bytes_to_hex(salt_bytes);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to generate salt: " + std::string(e.what()));
    }
}

// 字节转十六进制字符串（便于存储）
std::string JWT::bytes_to_hex(const std::string & bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : bytes) {
        ss << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

JWT *JWT::getInstance() {
    if (instance_== nullptr) {
        std::cerr<<"JWT instance is not initialized"<<std::endl;
    }
    return instance_;
}

// 补充：十六进制字符串转字节数组（解析盐值和哈希时需要）
std::string JWT::hex_to_bytes(const std::string& hex) {
    std::string bytes;
    if (hex.size() % 2 != 0) {
        return bytes; // 无效的十六进制字符串（长度为奇数）
    }
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        // 解析每两个字符为一个字节（16进制）
        std::string byte_str = hex.substr(i, 2);
        auto byte = static_cast<char>(std::stoul(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// 常量时间比较（防时序攻击）：无论内容是否相同，比较时间一致
bool JWT::constant_time_compare(const std::string & a, const std::string & b) {
    if (a.size() != b.size()) {
        std::cout<<"长度不同"<<std::endl;
        return false; // 长度不同直接返回false
    }
    int diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        // 异或：若字节不同则结果非0，累积到diff中
        diff |= (a[i] ^ b[i]);
    }
    return diff == 0; // 只有所有字节都相同，diff才为0
}

/**
 * 密码验证
 * @param password 明文密码
 * @param stored_hash 加密后存储的字符串（格式：salt_hex:iterations:hash_hex）
 * @return 是否验证通过
 */
bool JWT::verify_password(const std::string& password, const std::string& stored_hash) {
    // 1. 解析存储的哈希字符串（格式：salt_hex:iterations:hash_hex）
    size_t colon1 = stored_hash.find(':');
    size_t colon2 = stored_hash.find(':', colon1 + 1);
    if (colon1 == std::string::npos || colon2 == std::string::npos) {
        std::cout<<"格式错误"<<std::endl;
        return false; // 格式错误
    }

    // 提取盐值（十六进制）、迭代次数、存储的哈希（十六进制）
    std::string salt_hex = stored_hash.substr(0, colon1);
    std::string iterations_str = stored_hash.substr(colon1 + 1, colon2 - colon1 - 1);
    std::string stored_hash_hex = stored_hash.substr(colon2 + 1);

    // 2. 转换盐值和存储的哈希为字节数组
    std::string salt = hex_to_bytes(salt_hex);
    std::string expected_hash = hex_to_bytes(stored_hash_hex);
    if (salt.empty() || expected_hash.empty()) {
        std::cout<<"无效的十六进制"<<std::endl;
        return false; // 解析失败（无效的十六进制）
    }

    // 3. 解析迭代次数（确保为正整数）
    size_t iterations;
    try {
        iterations = std::stoul(iterations_str);
        if (iterations == 0) {
            std::cout<<"迭代次数不能为0"<<std::endl;
            return false; // 迭代次数不能为0
        }
    } catch (...) {
        std::cout<<"转换失败（非数字）"<<std::endl;
        return false; // 转换失败（非数字）
    }

    // 4. 用相同的盐值和迭代次数重新计算哈希
    std::string computed_hash(32, 0x00); // 输出32字节（SHA-256长度）

    // 复用PBKDF2-HMAC-SHA256逻辑（与加密时一致）
    std::string U, T(32, 0);
    for (size_t i = 1; i <= 1; ++i) { // 1个块（32字节）
        std::string salt_i = salt;
        // 添加4字节大端序号（与加密时一致，i=1）
        salt_i.push_back(static_cast<char>((i >> 24) & 0xFF));
        salt_i.push_back(static_cast<char>((i >> 16) & 0xFF));
        salt_i.push_back(static_cast<char>((i >> 8) & 0xFF));
        salt_i.push_back(static_cast<char>(i & 0xFF));

        U = hmacSha256(password, salt_i);
        T = U;
        for (size_t j = 1; j < iterations; ++j) {
            U = hmacSha256(password, U);
            for (size_t k = 0; k < 32; ++k) {
                T[k] ^= U[k];
            }
        }
        computed_hash = T;
    }

    // 5. 常量时间比较计算出的哈希与存储的哈希
    return constant_time_compare(computed_hash, expected_hash);
}