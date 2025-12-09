/**
 * @file JsonValue.cpp
 * @brief JSON值类型实现文件
 * @brief JSON Value Type Implementation File
 * 
 * 此文件实现了JsonValue类，用于处理JSON数据类型的解析和序列化，
 * 支持null、字符串、数字、布尔值、对象和数组等JSON数据类型。
 * This file implements the JsonValue class for handling JSON data types
 * parsing and serialization, supporting null, string, number, boolean,
 * object and array JSON data types.
 */
//
// Created by HP on 2025/11/5.
//

#include "../include/JsonValue.h"
#include <cstdio>
#include <utility>
#include <iostream>

// ========== JsonValue 实现 ==========
// ========== JsonValue Implementation ==========

/**
 * @brief 默认构造函数，创建null类型的JSON值
 * @brief Default constructor, creates a null type JSON value
 */
JsonValue::JsonValue() : type_(NULL_TYPE), numberValue_(0), boolValue_(false) {}

/**
 * @brief 从字符串构造JSON值
 * @param value 字符串值
 * @brief Constructor from string
 * @param value String value
 */
JsonValue::JsonValue(std::string  value) : type_(STRING), stringValue_(std::move(value)), numberValue_(0), boolValue_(false) {}

/**
 * @brief 从C风格字符串构造JSON值
 * @param value C风格字符串值
 * @brief Constructor from C-style string
 * @param value C-style string value
 * @note 如果传入nullptr，则创建空字符串
 * @note Creates empty string if nullptr is passed
 */
JsonValue::JsonValue(const char* value) : type_(STRING), stringValue_(value ? value : ""), numberValue_(0), boolValue_(false) {}

/**
 * @brief 从整数构造JSON值
 * @param value 整数值
 * @brief Constructor from integer
 * @param value Integer value
 */
JsonValue::JsonValue(int value) : type_(NUMBER), numberValue_(value), boolValue_(false) {}

/**
 * @brief 从浮点数构造JSON值
 * @param value 浮点数值
 * @brief Constructor from double
 * @param value Double value
 */
JsonValue::JsonValue(double value) : type_(NUMBER), numberValue_(value), boolValue_(false) {}

/**
 * @brief 从布尔值构造JSON值
 * @param value 布尔值
 * @brief Constructor from boolean
 * @param value Boolean value
 */
JsonValue::JsonValue(bool value) : type_(BOOLEAN), numberValue_(0), boolValue_(value) {}

/**
 * @brief 从对象映射构造JSON值
 * @param value 对象映射
 * @brief Constructor from object map
 * @param value Object map
 */
JsonValue::JsonValue(const std::map<std::string, JsonValue>& value) : type_(OBJECT), numberValue_(0), boolValue_(false), objectValue_(value) {}

/**
 * @brief 从数组构造JSON值
 * @param value JSON值数组
 * @brief Constructor from array
 * @param value JsonValue array
 */
JsonValue::JsonValue(const std::vector<JsonValue>& value) : type_(ARRAY), numberValue_(0), boolValue_(false), arrayValue_(value) {}

JsonValue::JsonValue(const std::vector<std::map<std::string, std::string>>& value) : type_(ARRAY), numberValue_(0), boolValue_(false) {
    for (const auto& item : value) {
        std::map<std::string, JsonValue> obj;
        for (const auto& pair : item) {
            obj[pair.first] = JsonValue(pair.second);
        }
        arrayValue_.emplace_back(obj);
    }
}

JsonValue::JsonValue(const std::vector<std::string>& value) : type_(ARRAY), numberValue_(0), boolValue_(false) {
    for (const auto& item : value) {
        arrayValue_.emplace_back(item);
    }
}

/**
 * @brief 获取字符串值
 * @return 如果是字符串类型，返回字符串值；否则返回空字符串
 * @brief Get string value
 * @return String value if string type, otherwise empty string
 */
std::string JsonValue::asString() const {
    if (type_ != STRING) return "";
    return stringValue_;
}

/**
 * @brief 获取数值
 * @return 数值
 * @brief Get number value
 * @return Number value
 */
double JsonValue::asNumber() const {
    return numberValue_;
}

/**
 * @brief 获取布尔值
 * @return 布尔值
 * @brief Get boolean value
 * @return Boolean value
 */
bool JsonValue::asBoolean() const {
    return boolValue_;
}

/**
 * @brief 获取对象值
 * @return 对象映射
 * @brief Get object value
 * @return Object map
 */
std::map<std::string, JsonValue> JsonValue::asObject() const {
    return objectValue_;
}

/**
 * @brief 获取数组值
 * @return JSON值数组
 * @brief Get array value
 * @return JsonValue array
 */
std::vector<JsonValue> JsonValue::asArray() const {
    return arrayValue_;
}

/**
 * @brief 转义JSON字符串中的特殊字符
 * @param str 原始字符串
 * @return 转义后的字符串
 * @brief Escape special characters in JSON string
 * @param str Original string
 * @return Escaped string
 * @note 对双引号、反斜杠、控制字符等特殊字符进行转义
 * @note Escapes double quotes, backslashes, control characters and other special characters
 */
std::string JsonValue::escapeJsonString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // 控制字符，使用Unicode转义
                    // Control characters, use Unicode escape
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    escaped += buf;
                } else {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

/**
 * @brief 将JSON值序列化为字符串
 * @return JSON字符串表示
 * @brief Serialize JSON value to string
 * @return JSON string representation
 * @note 根据不同的JSON类型，生成对应的JSON字符串格式
 * @note Generates corresponding JSON string format based on different JSON types
 */
std::string JsonValue::toJson() const {
    switch (type_) {
        case NULL_TYPE:
            return "null";
        case STRING:
            return "\"" + escapeJsonString(stringValue_) + "\"";
        case NUMBER:
            // 如果是整数，不显示小数点
            // If it's an integer, don't show decimal point
            if (numberValue_ == static_cast<int>(numberValue_)) {
                return std::to_string(static_cast<int>(numberValue_));
            }
            return std::to_string(numberValue_);
        case BOOLEAN:
            return boolValue_ ? "true" : "false";
        case OBJECT: {
            std::string json = "{";
            bool first = true;
            for (const auto& pair : objectValue_) {
                if (!first) json += ",";
                json += "\"" + escapeJsonString(pair.first) + "\":" + pair.second.toJson();
                first = false;
            }
            json += "}";
            return json;
        }
        case ARRAY: {
            std::string json = "[";
            bool first = true;
            for (const auto& item : arrayValue_) {
                if (!first) json += ",";
                json += item.toJson();
                first = false;
            }
            json += "]";
            return json;
        }
        default:
            return "null";
    }
}

/**
 * @brief 创建JSON对象的静态方法
 * @param obj 对象映射
 * @return JSON对象值
 * @brief Static method to create JSON object
 * @param obj Object map
 * @return JSON object value
 */
JsonValue JsonValue::object(const std::map<std::string, JsonValue>& obj) {
    return JsonValue(obj);
}

/**
 * @brief 创建JSON数组的静态方法
 * @param arr JSON值数组
 * @return JSON数组值
 * @brief Static method to create JSON array
 * @param arr JsonValue array
 * @return JSON array value
 */
JsonValue JsonValue::array(const std::vector<JsonValue>& arr) {
    return JsonValue(arr);
}

/**
 * @brief 将对象映射转换为JSON字符串
 * @param obj 对象映射
 * @return JSON字符串
 * @brief Convert object map to JSON string
 * @param obj Object map
 * @return JSON string
 */
std::string toJson(const std::map<std::string, JsonValue>& obj) {
    JsonValue value(obj);
    return value.toJson();
}

/**
 * @brief 将字符串映射数组转换为JSON数组字符串
 * @param vec 字符串映射数组
 * @return JSON数组字符串
 * @brief Convert string map array to JSON array string
 * @param vec String map array
 * @return JSON array string
 */
std::string toJsonArray(const std::vector<std::map<std::string, std::string>>& vec) {
    JsonValue value(vec);
    return value.toJson();
}

/**
 * @brief 从JSON字符串解析JSON值
 * @param jsonStr JSON字符串
 * @brief Parse JSON value from JSON string
 * @param jsonStr JSON string
 * @exception std::runtime_error 当JSON解析失败时抛出异常
 * @exception std::runtime_error Throws exception when JSON parsing fails
 * @note 解析失败时会重置为null类型
 * @note Resets to null type on parsing failure
 */
void JsonValue::fromJson(const std::string& jsonStr) {
    size_t pos = 0;
    try {
        // 递归解析整个JSON字符串，得到一个完整的JsonValue
        // Recursively parse the entire JSON string to get a complete JsonValue
        JsonValue parsed = parseValue(jsonStr, pos);

        // 检查解析是否完全（避免JSON字符串后有多余字符）
        // Check if parsing is complete (avoid extra characters after JSON value)
        pos = skipWhitespace(jsonStr, pos);
        if (pos < jsonStr.size()) {
            throw std::invalid_argument("Unexpected characters after JSON value");
        }

        // 将解析结果赋值给当前对象
        // Assign parsed result to current object
        this->type_ = parsed.type_;
        this->stringValue_ = std::move(parsed.stringValue_);
        this->numberValue_ = parsed.numberValue_;
        this->boolValue_ = parsed.boolValue_;
        this->objectValue_ = std::move(parsed.objectValue_);
        this->arrayValue_ = std::move(parsed.arrayValue_);
    } catch (const std::exception& e) {
        // 解析失败时，重置为null类型
        // On parsing failure, reset to null type
        this->type_ = NULL_TYPE;
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }
}
