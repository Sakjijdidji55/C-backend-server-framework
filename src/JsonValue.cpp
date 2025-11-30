//
// Created by HP on 2025/11/5.
//

#include "../include/JsonValue.h"
#include <cstdio>
#include <utility>

// ========== JsonValue 实现 ==========

JsonValue::JsonValue() : type_(NULL_TYPE), numberValue_(0), boolValue_(false) {}

JsonValue::JsonValue(std::string  value) : type_(STRING), stringValue_(std::move(value)), numberValue_(0), boolValue_(false) {}

JsonValue::JsonValue(const char* value) : type_(STRING), stringValue_(value ? value : ""), numberValue_(0), boolValue_(false) {}

JsonValue::JsonValue(int value) : type_(NUMBER), numberValue_(value), boolValue_(false) {}

JsonValue::JsonValue(double value) : type_(NUMBER), numberValue_(value), boolValue_(false) {}

JsonValue::JsonValue(bool value) : type_(BOOLEAN), numberValue_(0), boolValue_(value) {}

JsonValue::JsonValue(const std::map<std::string, JsonValue>& value) : type_(OBJECT), numberValue_(0), boolValue_(false), objectValue_(value) {}

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

std::string JsonValue::asString() const {
    if (type_ != STRING) return "";
    return stringValue_;
}

double JsonValue::asNumber() const {
    return numberValue_;
}

bool JsonValue::asBoolean() const {
    return boolValue_;
}

std::map<std::string, JsonValue> JsonValue::asObject() const {
    return objectValue_;
}

std::vector<JsonValue> JsonValue::asArray() const {
    return arrayValue_;
}

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

std::string JsonValue::toJson() const {
    switch (type_) {
        case NULL_TYPE:
            return "null";
        case STRING:
            return "\"" + escapeJsonString(stringValue_) + "\"";
        case NUMBER:
            // 如果是整数，不显示小数点
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

JsonValue JsonValue::object(const std::map<std::string, JsonValue>& obj) {
    return JsonValue(obj);
}

JsonValue JsonValue::array(const std::vector<JsonValue>& arr) {
    return JsonValue(arr);
}

std::string toJson(const std::map<std::string, JsonValue>& obj) {
    JsonValue value(obj);
    return value.toJson();
}

std::string toJsonArray(const std::vector<std::map<std::string, std::string>>& vec) {
    JsonValue value(vec);
    return value.toJson();
}

