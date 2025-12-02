//
// Created by HP on 2025/11/13.
//

#include "../include/jwt.h"
//#include "../include/config.h"

#include <array>
#include <cstring>
#include <iomanip>
#include <mutex>

namespace {

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

inline std::uint32_t rotr(std::uint32_t value, std::uint32_t count) {
    return (value >> count) | (value << (32 - count));
}

} // namespace

JWT* JWT::instance_ = nullptr;

/**
 * @brief JWT类的构造函数，初始化JWT实例
 *
 * 该构造函数用于创建JWT对象，设置默认算法为HS256，默认生存时间(TTL)为3600秒。
 * 它会检查是否已经存在JWT实例，如果存在则输出错误信息。
 * 同时，如果配置存在，会尝试从配置中加载JWT的过期时间和RSA私钥路径。
 *
 * @note 这是一个单例模式的实现，确保系统中只有一个JWT实例
 */
JWT::JWT(const std::string& expiresIn, const std::string& jwtRsaPrivateKeyPath) : algorithm_(Algorithm::HS256), defaultTTL_(3600) {
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

    if (!jwtRsaPrivateKeyPath.empty()) {
        loadSecretFromFile(jwtRsaPrivateKeyPath);
    }
    instance_ = this;
}

JWT::JWT(std::string secret, long long ttlSeconds)
        : secret_(std::move(secret)), algorithm_(Algorithm::HS256), defaultTTL_(ttlSeconds > 0 ? ttlSeconds : 3600) {
    if (secret_.empty()) {
        throw std::invalid_argument("JWT secret must not be empty");
    }
}

void JWT::setSecret(const std::string& secret) {
    if (secret.empty()) {
        throw std::invalid_argument("JWT secret must not be empty");
    }
    secret_ = secret;
}

/**
 * 从文件中加载密钥
 * @param path 密钥文件的路径
 * @throws std::runtime_error 当文件无法打开时抛出异常
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
    std::ostringstream oss;
    // 将文件内容读取到字符串流中
    oss << file.rdbuf();
    // 设置读取到的内容为密钥
//    std::cout<<oss.str()<<std::endl;
    setSecret(oss.str());
}

void JWT::setDefaultTTL(long long ttlSeconds) {
    if (ttlSeconds <= 0) {
        throw std::invalid_argument("TTL must be positive");
    }
    defaultTTL_ = ttlSeconds;
}

/**
 * 生成JWT令牌
 * @param customClaims 自定义声明，一个键值对映射
 * @param ttlSeconds 令牌有效期（秒），-1表示使用默认有效期
 * @return 生成的JWT令牌字符串
 * @throws std::runtime_error 当密钥未设置时抛出异常
 */
std::string JWT::generateToken(const std::map<std::string, std::string>& customClaims, long long ttlSeconds) const {
    // 检查密钥是否已设置
    if (secret_.empty()) {
//        std::cout<<"empty"<<std::endl;
        throw std::runtime_error("JWT secret is not set");
    }

    // 构建并编码JWT头部
    const auto header = buildHeader();
    const auto headerEncoded = base64UrlEncode(header);

    // 获取当前时间戳（秒）
    auto now = std::chrono::system_clock::now();
    auto issuedAt = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // 处理令牌有效期
    long long effectiveTTL = ttlSeconds;
    if (effectiveTTL < 0) {
        effectiveTTL = defaultTTL_;  // 使用默认有效期
    }
    std::optional<long long> expiresAt;
    if (effectiveTTL > 0) {
        expiresAt = issuedAt + effectiveTTL;  // 计算过期时间戳
    }

    // 构建并编码JWT载荷
    const auto payload = buildPayload(customClaims, issuedAt, expiresAt);
    const auto payloadEncoded = base64UrlEncode(payload);

    // 创建签名输入并生成签名
    const std::string signingInput = headerEncoded + "." + payloadEncoded;
    const auto signature = base64UrlEncode(hmacSha256(signingInput, secret_));

    // 组合头部、载荷和签名，返回完整的JWT令牌
    return signingInput + "." + signature;
}

/**
 * 验证JWT令牌的有效性
 * @param token 待验证的JWT令牌字符串
 * @param payloadJson 用于存储载荷JSON字符串的输出参数，可为nullptr
 * @return 验证成功返回true，失败返回false
 */
bool JWT::verifyToken(const std::string& token, std::string* payloadJson) const {
    // 检查密钥是否为空
    if (secret_.empty()) {
        return false;
    }

    // 分割令牌为三部分（头部、载荷、签名）
    // Split token into three parts (header, payload, signature)
    const auto parts = splitToken(token);
    // 检查令牌是否由三部分组成
    if (parts.size() != 3) {
        return false;
    }

    // 构造待签名的字符串（头部+载荷）
    const std::string signingInput = parts[0] + "." + parts[1];
    // 计算期望的签名
    const std::string expectedSignature = base64UrlEncode(hmacSha256(signingInput, secret_));
    // 验证签名是否匹配
    if (expectedSignature != parts[2]) {
        return false;
    }

    // 解码载荷部分
    const std::string payload = base64UrlDecode(parts[1]);

    // 检查过期时间(exp)声明
    const auto expClaim = extractNumericClaim(payload, "exp");
    if (expClaim.has_value()) {
        // 获取当前时间戳
        auto now = std::chrono::system_clock::now();
        auto current = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        // 检查是否过期
        if (current > expClaim.value()) {
            return false;
        }
    }

    // 如果提供了payloadJson参数，则将载荷JSON字符串存入
    if (payloadJson != nullptr) {
        *payloadJson = payload;
    }
    return true;
}

/**
 * @brief 解析JWT令牌中的声明（claims）部分
 * @param token 要解析的JWT令牌字符串
 * @return 返回一个包含字符串映射的std::optional对象，如果解析失败则返回std::nullopt
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

JWT::Algorithm JWT::algorithm() const {
    return algorithm_;
}

/**
 * @brief JWT类的base64UrlEncode方法
 * @param data 需要进行base64Url编码的原始数据
 * @return 返回编码后的字符串
 *
 * 该方法实现了base64Url编码，它是一种base64编码的变体，主要用于URL安全。
 * 与标准base64编码相比，它将'+'替换为'-'，'/'替换为'_'，并移除了末尾的'='填充字符。
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
 * @param data 需要解码的Base64URL字符串
 * @return 解码后的原始字符串
 *
 * 该函数将Base64URL编码的字符串转换为原始字符串。Base64URL是Base64的变种，
 * 使用'-'和'_'替代了标准的'+'和'/'，同时不使用填充字符'='。
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
 * @param data 输入字符串
 * @return 返回计算得到的SHA256哈希值字符串
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
            std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
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
            std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            std::uint32_t ch = (e & f) ^ ((~e) & g);
            std::uint32_t temp1 = h + s1 + ch + kSha256Constants[i] + w[i];
            std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
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
 * HMAC-SHA256 签名算法实现
 * HMAC (Hash-based Message Authentication Code) 是一种基于哈希的消息认证码算法
 * 此函数使用SHA-256作为哈希函数实现HMAC签名
 *
 * @param data 要签名的原始数据
 * @param key 用于签名的密钥
 * @return 返回HMAC-SHA256签名结果
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
 * 构建JWT头部信息
 * @return 返回包含算法和类型的标准JWT头部字符串
 *
 * 该函数返回一个固定的JWT头部字符串，使用HS256算法作为签名算法，
 * 并指定令牌类型为JWT(JSON Web Token)
 */
std::string JWT::buildHeader() {
    // 使用静态常量存储标准JWT头部字符串
    // 内容为JSON格式，包含算法(alg)和类型(typ)字段
    static const std::string header = R"({"alg":"HS256","typ":"JWT"})";
    // 返回预定义的JWT头部
    return header;
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
 * 构建JWT的payload部分
 * @param customClaims 自定义声明键值对集合
 * @param issuedAt 签发时间(Unix时间戳)
 * @param expiresAt 过期时间(Unix时间戳)，可选参数
 * @return 返回格式化的JSON字符串作为JWT的payload
 */
std::string JWT::buildPayload(const std::map<std::string, std::string>& customClaims,
                              long long issuedAt,  // 签发时间(Unix时间戳)
                              std::optional<long long> expiresAt) {  // 过期时间(Unix时间戳)，可选
    // 使用字符串流构建JSON
    std::ostringstream oss;
    oss << "{";  // 开始JSON对象
    // 添加签发时间(iat)字段
    oss << "\"iat\":" << issuedAt;
    // 如果提供了过期时间，添加过期时间(exp)字段
    if (expiresAt.has_value()) {
        oss << ",\"exp\":" << expiresAt.value();
    }

    // 遍历自定义声明集合，添加到JSON中
    for (const auto& [key, value] : customClaims) {
        // 对键和值进行JSON转义后添加到JSON字符串中
        oss << ",\"" << escapeJson(key) << "\":\"" << escapeJson(value) << "\"";
    }

    oss << "}";  // 结束JSON对象
    return oss.str();  // 返回构建完成的JSON字符串
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
 * @brief 解析JSON对象字符串为键值对映射
 *
 * 此函数将JSON格式的字符串解析为std::map<std::string, std::string>，
 * 支持字符串类型的键值对，处理转义字符，并进行基本的错误检查。
 *
 * @param json 输入的JSON格式字符串
 * @return std::map<std::string, std::string> 解析后的键值对映射
 * @throws std::runtime_error 当JSON格式无效时抛出异常
 */
std::map<std::string, std::string> JWT::parseJsonObject(const std::string& json) {
    // 存储解析结果的键值对映射
    std::map<std::string, std::string> result;
    // 字符串解析的当前位置索引
    std::size_t i = 0;

    // 定义一个lambda函数用于跳过空白字符
    auto skipWhitespace = [&](std::size_t& idx) {
        while (idx < json.size() && std::isspace(static_cast<unsigned char>(json[idx]))) {
            ++idx;
        }
    };

    // 跳过开头空白并检查是否为对象开始标记'{'
    skipWhitespace(i);
    if (i >= json.size() || json[i] != '{') {
        throw std::runtime_error("Invalid JSON object: missing '{'");
    }
    ++i;

    // 循环解析JSON对象的键值对
    while (true) {
        // 跳过空白并检查当前位置
        skipWhitespace(i);
        if (i >= json.size()) {
            throw std::runtime_error("Invalid JSON object: unexpected end");
        }
        // 检查是否到达对象结束
        if (json[i] == '}') {
            ++i;
            break;
        }
        // 检查键是否以双引号开始
        if (json[i] != '"') {
            throw std::runtime_error("Invalid JSON object: expected string key");
        }
        ++i;
        // 解析键
        std::string key;
        while (i < json.size()) {
            char ch = json[i++];
            if (ch == '\\') {
                // 处理转义字符
                if (i >= json.size()) {
                    throw std::runtime_error("Invalid JSON object: bad escape in key");
                }
                char esc = json[i++];
                switch (esc) {
                    case '"': key.push_back('"'); break;
                    case '\\': key.push_back('\\'); break;
                    case '/': key.push_back('/'); break;
                    case 'b': key.push_back('\b'); break;
                    case 'f': key.push_back('\f'); break;
                    case 'n': key.push_back('\n'); break;
                    case 'r': key.push_back('\r'); break;
                    case 't': key.push_back('\t'); break;
                    case 'u': {
                        // 处理Unicode转义序列
                        if (i + 4 > json.size()) {
                            throw std::runtime_error("Invalid JSON object: bad unicode escape");
                        }
                        std::string hex = json.substr(i, 4);
                        i += 4;
                        auto code = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
                        if (code <= 0x7F) {
                            key.push_back(static_cast<char>(code));
                        } else if (code <= 0x7FF) {
                            key.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
                            key.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        } else {
                            key.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
                            key.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                            key.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("Invalid JSON object: unsupported escape in key");
                }
            } else if (ch == '"') {
                // 键结束
                break;
            } else {
                // 普通字符
                key.push_back(ch);
            }
        }

        // 检查键值分隔符':'
        skipWhitespace(i);
        if (i >= json.size() || json[i] != ':') {
            throw std::runtime_error("Invalid JSON object: expected ':'");
        }
        ++i;

        // 跳过空白字符准备解析值
        skipWhitespace(i);
        if (i >= json.size()) {
            throw std::runtime_error("Invalid JSON object: missing value");
        }

        // 解析值
        std::string value;
        if (json[i] == '"') {
            // 字符串类型的值
            ++i;
            std::string raw;
            while (i < json.size()) {
                char ch = json[i++];
                if (ch == '\\') {
                    // 处理值中的转义字符
                    if (i >= json.size()) {
                        throw std::runtime_error("Invalid JSON object: bad escape in value");
                    }
                    char esc = json[i++];
                    raw.push_back('\\');
                    raw.push_back(esc);
                } else if (ch == '"') {
                    // 值结束
                    break;
                } else {
                    // 普通字符
                    raw.push_back(ch);
                }
            }
            value = unescapeJson(raw);
        } else {
            // 非字符串类型的值
            std::size_t start = i;
            while (i < json.size() && json[i] != ',' && json[i] != '}') {
                ++i;
            }
            value = json.substr(start, i - start);
            // 移除值末尾的空白字符
            std::size_t endpos = value.find_last_not_of(" \t\n\r");
            if (endpos == std::string::npos) {
                value.clear();
            } else {
                value.erase(endpos + 1);
            }
        }

        // 将解析的键值对添加到结果中
        result.emplace(std::move(key), std::move(value));

        // 检查是否还有更多键值对
        skipWhitespace(i);
        if (i >= json.size()) {
            throw std::runtime_error("Invalid JSON object: unexpected end after value");
        }
        if (json[i] == ',') {
            // 还有更多键值对
            ++i;
            continue;
        }
        if (json[i] == '}') {
            // 对象结束
            ++i;
            break;
        }
        throw std::runtime_error("Invalid JSON object: expected ',' or '}'");
    }

    // 检查是否还有未处理的字符
    skipWhitespace(i);
    if (i != json.size()) {
        throw std::runtime_error("Invalid JSON object: trailing characters");
    }

    return result;
}

/**
 * @brief 对经过JSON转义的字符串进行反转义处理
 * @param input 已经经过JSON转义的输入字符串
 * @return 返回反转义后的原始字符串
 * @throws std::runtime_error 当遇到无效的转义序列或Unicode转义时抛出异常
 */
std::string JWT::unescapeJson(const std::string& input) {
    std::string output;  // 用于存储反转义后的结果字符串
    output.reserve(input.size());  // 预分配足够的内存空间以提高性能
    // 遍历输入字符串的每个字符
    for (std::size_t i = 0; i < input.size();) {
        char ch = input[i++];
        // 如果不是转义字符，直接添加到输出结果中
        if (ch != '\\') {
            output.push_back(ch);
            continue;
        }
        // 检查是否到达字符串末尾，避免越界访问
        if (i >= input.size()) {
            throw std::runtime_error("Invalid escape sequence");
        }
        char esc = input[i++];  // 获取转义字符
        // 根据不同的转义字符进行处理
        switch (esc) {
            case '"': output.push_back('"'); break;  // 双引号
            case '\\': output.push_back('\\'); break;  // 反斜杠
            case '/': output.push_back('/'); break;  // 正斜杠
            case 'b': output.push_back('\b'); break;  // 退格
            case 'f': output.push_back('\f'); break;  // 换页
            case 'n': output.push_back('\n'); break;  // 换行
            case 'r': output.push_back('\r'); break;  // 回车
            case 't': output.push_back('\t'); break;  // 制表符
            case 'u': {  // Unicode转义序列处理
                // 检查是否有足够的字符用于Unicode转义
                if (i + 4 > input.size()) {
                    throw std::runtime_error("Invalid unicode escape");
                }
                // 提取4位十六进制数字
                std::string hex = input.substr(i, 4);
                i += 4;
                // 将十六进制字符串转换为字符码
                auto code = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
                // 根据Unicode码点范围进行UTF-8编码
                if (code <= 0x7F) {
                    output.push_back(static_cast<char>(code));
                } else if (code <= 0x7FF) {
                    output.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
                    output.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                } else {
                    output.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
                    output.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                    output.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                }
                break;
            }
            default:
                throw std::runtime_error("Invalid escape sequence");
        }
    }
    return output;
}

/**
     * 安全的密码加密：使用PBKDF2-HMAC-SHA256
     * @param password 明文密码
     * @return 存储格式：salt(hex):iterations:hash(hex)
     */
std::string JWT::encrypt_password(const std::string& password) {
    const size_t iterations = 100000; // 迭代次数（可根据性能调整）
    std::vector<uint8_t> salt = generate_salt();
    std::vector<uint8_t> password_bytes(password.begin(), password.end());

    // PBKDF2核心：多次迭代HMAC-SHA256
    std::vector<uint8_t> dk(32); // 输出32字节哈希
    std::vector<uint8_t> U, T(32, 0);
    for (size_t i = 1; i <= 1; ++i) { // 只需要1个块（32字节）
        // 盐值 + 4字节大端序号（i=1）
        std::vector<uint8_t> salt_i = salt;
        salt_i.push_back((i >> 24) & 0xFF);
        salt_i.push_back((i >> 16) & 0xFF);
        salt_i.push_back((i >> 8) & 0xFF);
        salt_i.push_back(i & 0xFF);

        U = hmac_sha256(password_bytes, salt_i);
        T = U;
        for (size_t j = 1; j < iterations; ++j) {
            U = hmac_sha256(password_bytes, U);
            for (size_t k = 0; k < 32; ++k) {
                T[k] ^= U[k]; // 异或累积
            }
        }
        dk = T;
    }

    return bytes_to_hex(salt) + ":" + std::to_string(iterations) + ":" + bytes_to_hex(dk);
}

// HMAC-SHA256实现（基于sha256_bytes）
std::vector<uint8_t> JWT::hmac_sha256(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data
) {
    std::vector<uint8_t> key_padded(64, 0x00);
    if (key.size() > 64) {
        key_padded = sha256_bytes(key); // 密钥过长时先哈希
    } else {
        memcpy(key_padded.data(), key.data(), key.size());
    }

    std::vector<uint8_t> ipad(64), opad(64);
    for (int i = 0; i < 64; ++i) {
        ipad[i] = key_padded[i] ^ 0x36;
        opad[i] = key_padded[i] ^ 0x5c;
    }

    // 计算内部哈希：HMAC(key ^ ipad, data)
    std::vector<uint8_t> inner_data = ipad;
    inner_data.insert(inner_data.end(), data.begin(), data.end());
    std::vector<uint8_t> inner_hash = sha256_bytes(inner_data);

    // 计算外部哈希：HMAC(key ^ opad, inner_hash)
    std::vector<uint8_t> outer_data = opad;
    outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
    return sha256_bytes(outer_data);
}

// 生成随机盐值（16字节，足够安全）
std::vector<uint8_t> JWT::generate_salt() {
    std::vector<uint8_t> salt(16);
    // 使用加密安全的随机数生成器（跨平台实现需调整）
    std::random_device rd;
    std::mt19937_64 gen(rd());
    for (auto& b : salt) {
        b = static_cast<uint8_t>(gen() % 256);
    }
    return salt;
}

// 字节转十六进制字符串（便于存储）
std::string JWT::bytes_to_hex(const std::vector<uint8_t>& bytes) {
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

std::vector<uint8_t> JWT::sha256_bytes(const std::vector<uint8_t> &data) {
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
            std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
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
            std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            std::uint32_t ch = (e & f) ^ ((~e) & g);
            std::uint32_t temp1 = h + s1 + ch + kSha256Constants[i] + w[i];
            std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
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
    std::vector<uint8_t> result;
    result.reserve(32);
    for (std::uint32_t part : hash) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            result.push_back(static_cast<char>((part >> shift) & 0xFF));
        }
    }
    return result;
}

// 补充：十六进制字符串转字节数组（解析盐值和哈希时需要）
std::vector<uint8_t> JWT::hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    if (hex.size() % 2 != 0) {
        return bytes; // 无效的十六进制字符串（长度为奇数）
    }
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        // 解析每两个字符为一个字节（16进制）
        std::string byte_str = hex.substr(i, 2);
        auto byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// 常量时间比较（防时序攻击）：无论内容是否相同，比较时间一致
bool JWT::constant_time_compare(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
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
    std::vector<uint8_t> salt = hex_to_bytes(salt_hex);
    std::vector<uint8_t> expected_hash = hex_to_bytes(stored_hash_hex);
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
    std::vector<uint8_t> password_bytes(password.begin(), password.end());
    std::vector<uint8_t> computed_hash(32); // 输出32字节（SHA-256长度）

    // 复用PBKDF2-HMAC-SHA256逻辑（与加密时一致）
    std::vector<uint8_t> U, T(32, 0);
    for (size_t i = 1; i <= 1; ++i) { // 1个块（32字节）
        std::vector<uint8_t> salt_i = salt;
        // 添加4字节大端序号（与加密时一致，i=1）
        salt_i.push_back((i >> 24) & 0xFF);
        salt_i.push_back((i >> 16) & 0xFF);
        salt_i.push_back((i >> 8) & 0xFF);
        salt_i.push_back(i & 0xFF);

        U = hmac_sha256(password_bytes, salt_i);
        T = U;
        for (size_t j = 1; j < iterations; ++j) {
            U = hmac_sha256(password_bytes, U);
            for (size_t k = 0; k < 32; ++k) {
                T[k] ^= U[k];
            }
        }
        computed_hash = T;
    }

    // 5. 常量时间比较计算出的哈希与存储的哈希
    return constant_time_compare(computed_hash, expected_hash);
}