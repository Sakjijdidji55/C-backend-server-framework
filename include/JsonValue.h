//
// Created by HP on 2025/11/5.
//

#ifndef FLIGHTSERVER_JSONVALUE_H
#define FLIGHTSERVER_JSONVALUE_H

#include <string>
#include <map>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <algorithm>
/**
 * @brief JSON值类型类，支持嵌套结构的JSON数据
 * 
 * 该类提供了完整的JSON值表示，支持以下类型：
 * - NULL_TYPE: null值
 * - STRING: 字符串值
 * - NUMBER: 数字值（整数或浮点数）
 * - BOOLEAN: 布尔值
 * - OBJECT: 对象（键值对映射）
 * - ARRAY: 数组
 * 
 * @brief JSON value type class that supports nested structured JSON data
 * 
 * This class provides complete JSON value representation, supporting the following types:
 * - NULL_TYPE: null value
 * - STRING: string value
 * - NUMBER: numeric value (integer or floating point)
 * - BOOLEAN: boolean value
 * - OBJECT: object (key-value pair mapping)
 * - ARRAY: array
 */
class JsonValue {
public:
    /**
     * @brief JSON值类型枚举
     * @brief JSON value type enumeration
     */
    enum Type {
        NULL_TYPE,  ///< null类型 (null type)
        STRING,     ///< 字符串类型 (string type)
        NUMBER,     ///< 数字类型 (numeric type)
        BOOLEAN,    ///< 布尔类型 (boolean type)
        OBJECT,     ///< 对象类型 (object type)
        ARRAY       ///< 数组类型 (array type)
    };

    /**
     * @brief 默认构造函数，创建null值
     * @brief Default constructor, creates a null value
     */
    JsonValue();

    /**
     * @brief 从字符串构造JsonValue
     * @param value 字符串值
     * @brief Construct JsonValue from string
     * @param value String value
     */
    explicit JsonValue(std::string  value);

    /**
     * @brief 从C风格字符串构造JsonValue
     * @param value C风格字符串
     * @brief Construct JsonValue from C-style string
     * @param value C-style string
     */
    explicit JsonValue(const char* value);

    /**
     * @brief 从整数构造JsonValue
     * @param value 整数值
     * @brief Construct JsonValue from integer
     * @param value Integer value
     */
    explicit JsonValue(int value);

    /**
     * @brief 从浮点数构造JsonValue
     * @param value 浮点数值
     * @brief Construct JsonValue from floating-point number
     * @param value Floating-point value
     */
    explicit JsonValue(double value);

    /**
     * @brief 从布尔值构造JsonValue
     * @param value 布尔值
     * @brief Construct JsonValue from boolean
     * @param value Boolean value
     */
    explicit JsonValue(bool value);

    /**
     * @brief 从对象映射构造JsonValue
     * @param value 对象映射
     * @brief Construct JsonValue from object map
     * @param value Object map
     */
    explicit JsonValue(const std::map<std::string, JsonValue>& value);

    /**
     * @brief 从JsonValue数组构造JsonValue
     * @param value JsonValue数组
     * @brief Construct JsonValue from JsonValue array
     * @param value JsonValue array
     */
    explicit JsonValue(const std::vector<JsonValue>& value);

    /**
     * @brief 从map数组构造JsonValue
     * @param value map数组
     * @brief Construct JsonValue from map array
     * @param value Map array
     */
    explicit JsonValue(const std::vector<std::map<std::string, std::string>>& value);

    /**
     * @brief 从字符串数组构造JsonValue
     * @param value 字符串数组
     * @brief Construct JsonValue from string array
     * @param value String array
     */
    explicit JsonValue(const std::vector<std::string>& value);

    /**
     * @brief 获取值的类型
     * @return 值的类型
     * @brief Get the type of the value
     * @return The type of the value
     */
    [[nodiscard]] Type getType() const { return type_; }

    /**
     * @brief 获取字符串值
     * @return 字符串值，如果类型不是STRING则返回空字符串
     * @brief Get string value
     * @return String value, returns empty string if type is not STRING
     */
    [[nodiscard]] std::string asString() const;

    /**
     * @brief 获取数字值
     * @return 数字值
     */
    [[nodiscard]] double asNumber() const;

    /**
     * @brief 获取布尔值
     * @return 布尔值
     */
    [[nodiscard]] bool asBoolean() const;

    /**
     * @brief 获取对象值
     * @return 对象映射
     */
    [[nodiscard]] std::map<std::string, JsonValue> asObject() const;

    /**
     * @brief 获取数组值
     * @return JsonValue数组
     */
    [[nodiscard]] std::vector<JsonValue> asArray() const;

    /**
     * @brief 转换为JSON字符串
     * @return JSON格式的字符串
     */
    [[nodiscard]] std::string toJson() const;

    /**
    * @brief 从JSON字符串解析并填充对象属性
    * @param jsonStr 输入的JSON格式字符串，用于解析并填充对象的属性
    */
    void fromJson(const std::string& jsonStr);


    /**
     * @brief 静态方法：创建对象
     * @param obj 对象映射
     * @return JsonValue对象
     */
    static JsonValue object(const std::map<std::string, JsonValue>& obj);
    
    /**
     * @brief 静态方法：创建数组
     * @param arr JsonValue数组
     * @return JsonValue对象
     */
    static JsonValue array(const std::vector<JsonValue>& arr);

private:
    Type type_;                                    ///< 值的类型
    std::string stringValue_;                     ///< 字符串值
    double numberValue_;                          ///< 数字值
    bool boolValue_;                              ///< 布尔值
    std::map<std::string, JsonValue> objectValue_; ///< 对象值
    std::vector<JsonValue> arrayValue_;           ///< 数组值

    /**
     * @brief 转义JSON字符串中的特殊字符
     * @param str 原始字符串
     * @return 转义后的字符串
     */
    [[nodiscard]] static std::string escapeJsonString(const std::string& str) ;

    static size_t skipWhitespace(const std::string& jsonStr, size_t pos) {
        while (pos < jsonStr.size() && std::isspace(static_cast<unsigned char>(jsonStr[pos]))) {
            pos++;
        }
        return pos;
    }

// 辅助函数：解析字符串（处理转义字符）
    static std::string parseString(const std::string& jsonStr, size_t& pos) {
        pos++; // 跳过开头的 "
        std::string result;
        while (pos < jsonStr.size() && jsonStr[pos] != '"') {
            if (jsonStr[pos] == '\\') { // 处理转义字符
                pos++;
                if (pos >= jsonStr.size()) {
                    throw std::invalid_argument("Invalid JSON string: unexpected end after escape");
                }
                switch (jsonStr[pos]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: throw std::invalid_argument("Invalid escape character in JSON string");
                }
            } else {
                result += jsonStr[pos];
            }
            pos++;
        }
        if (pos >= jsonStr.size() || jsonStr[pos] != '"') {
            throw std::invalid_argument("Invalid JSON string: missing closing quote");
        }
        pos++; // 跳过结尾的 "
        return result;
    }

// 辅助函数：解析数字
    static double parseNumber(const std::string& jsonStr, size_t& pos) {
        size_t start = pos;
        // 处理负号
        if (jsonStr[pos] == '-') {
            pos++;
        }
        // 处理整数部分
        while (pos < jsonStr.size() && std::isdigit(static_cast<unsigned char>(jsonStr[pos]))) {
            pos++;
        }
        // 处理小数部分
        if (pos < jsonStr.size() && jsonStr[pos] == '.') {
            pos++;
            while (pos < jsonStr.size() && std::isdigit(static_cast<unsigned char>(jsonStr[pos]))) {
                pos++;
            }
        }
        // 处理指数部分
        if (pos < jsonStr.size() && (jsonStr[pos] == 'e' || jsonStr[pos] == 'E')) {
            pos++;
            if (pos < jsonStr.size() && (jsonStr[pos] == '+' || jsonStr[pos] == '-')) {
                pos++;
            }
            while (pos < jsonStr.size() && std::isdigit(static_cast<unsigned char>(jsonStr[pos]))) {
                pos++;
            }
        }
        // 转换为double
        std::string numStr = jsonStr.substr(start, pos - start);
        try {
            return std::stod(numStr);
        } catch (...) {
            throw std::invalid_argument("Invalid JSON number: " + numStr);
        }
    }

// 辅助函数：解析布尔值
    static bool parseBoolean(const std::string& jsonStr, size_t& pos) {
        if (jsonStr.substr(pos, 4) == "true") {
            pos += 4;
            return true;
        } else if (jsonStr.substr(pos, 5) == "false") {
            pos += 5;
            return false;
        } else {
            throw std::invalid_argument("Invalid JSON boolean value");
        }
    }

// 辅助函数：解析null
    static void parseNull(const std::string& jsonStr, size_t& pos) {
        if (jsonStr.substr(pos, 4) != "null") {
            throw std::invalid_argument("Invalid JSON null value");
        }
        pos += 4;
    }

// 辅助函数：解析对象（键值对集合）
    static std::map<std::string, JsonValue> parseObject(const std::string& jsonStr, size_t& pos) {
        pos++; // 跳过开头的 {
        std::map<std::string, JsonValue> obj;
        pos = skipWhitespace(jsonStr, pos);

        while (pos < jsonStr.size() && jsonStr[pos] != '}') {
            // 解析键（必须是字符串）
            if (jsonStr[pos] != '"') {
                throw std::invalid_argument("Invalid JSON object: key must be a string");
            }
            std::string key = parseString(jsonStr, pos);
            pos = skipWhitespace(jsonStr, pos);

            // 解析冒号
            if (pos >= jsonStr.size() || jsonStr[pos] != ':') {
                throw std::invalid_argument("Invalid JSON object: missing colon after key");
            }
            pos++;
            pos = skipWhitespace(jsonStr, pos);

            // 解析值（递归）
            JsonValue value = parseValue(jsonStr, pos);
            obj[key] = value;
            pos = skipWhitespace(jsonStr, pos);

            // 处理逗号或结束符
            if (pos < jsonStr.size() && jsonStr[pos] == ',') {
                pos++;
                pos = skipWhitespace(jsonStr, pos);
            } else if (jsonStr[pos] != '}') {
                throw std::invalid_argument("Invalid JSON object: unexpected character");
            }
        }

        if (pos >= jsonStr.size() || jsonStr[pos] != '}') {
            throw std::invalid_argument("Invalid JSON object: missing closing brace");
        }
        pos++; // 跳过结尾的 }
        return obj;
    }

// 辅助函数：解析数组（值列表）
    static std::vector<JsonValue> parseArray(const std::string& jsonStr, size_t& pos) {
        pos++; // 跳过开头的 [
        std::vector<JsonValue> arr;
        pos = skipWhitespace(jsonStr, pos);

        while (pos < jsonStr.size() && jsonStr[pos] != ']') {
            // 解析元素（递归）
            JsonValue elem = parseValue(jsonStr, pos);
            arr.push_back(elem);
            pos = skipWhitespace(jsonStr, pos);

            // 处理逗号或结束符
            if (pos < jsonStr.size() && jsonStr[pos] == ',') {
                pos++;
                pos = skipWhitespace(jsonStr, pos);
            } else if (jsonStr[pos] != ']') {
                throw std::invalid_argument("Invalid JSON array: unexpected character");
            }
        }

        if (pos >= jsonStr.size() || jsonStr[pos] != ']') {
            throw std::invalid_argument("Invalid JSON array: missing closing bracket");
        }
        pos++; // 跳过结尾的 ]
        return arr;
    }

    // 递归解析JSON值
    static JsonValue parseValue(const std::string& jsonStr, size_t& pos) {
        pos = skipWhitespace(jsonStr, pos);
        if (pos >= jsonStr.size()) {
            throw std::invalid_argument("Empty JSON string");
        }

        char c = jsonStr[pos];
        switch (c) {
            case '{': return JsonValue(parseObject(jsonStr, pos));
            case '[': return JsonValue(parseArray(jsonStr, pos));
            case '"': return JsonValue(parseString(jsonStr, pos));
            case 't': case 'f': return JsonValue(parseBoolean(jsonStr, pos));
            case 'n': parseNull(jsonStr, pos); return JsonValue(); // null类型
            case '-': case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return JsonValue(parseNumber(jsonStr, pos));
            default: throw std::invalid_argument("Invalid JSON value: unexpected character '" + std::string(1, c) + "'");
        }
    }
};

/**
 * @brief 便捷函数：将嵌套map转换为JSON字符串
 * @param obj 包含JsonValue的map
 * @return JSON格式的字符串
 */
std::string toJson(const std::map<std::string, JsonValue>& obj);

/**
 * @brief 便捷函数：将vector<map>转换为JSON数组字符串
 * @param vec map数组
 * @return JSON数组格式的字符串
 */
std::string toJsonArray(const std::vector<std::map<std::string, std::string>>& vec);

#endif //FLIGHTSERVER_JSONVALUE_H

