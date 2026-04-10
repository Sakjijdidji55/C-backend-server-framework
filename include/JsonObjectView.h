//
// Lightweight JSON object accessor: parse string once, use operator[] and optional unordered_map export.
//

#ifndef CBSF_JSON_OBJECT_VIEW_H
#define CBSF_JSON_OBJECT_VIEW_H

#include "JsonValue.h"

#include <string>
#include <unordered_map>
#include <utility>

/**
 * @brief 包装 JsonValue，提供 operator[] 链式下标访问；可将 object 一层导出为 unordered_map 便于遍历。
 * @note 非 object 的节点上 [] 得到空视图；asString 行为与原先 jsonGetString 一致（非 string 用 toJson）。
 */
class JsonObjectView {
public:
    JsonObjectView() = default;

    explicit JsonObjectView(JsonValue v) : node_(std::move(v)) {}

    /** 解析 JSON 文本为根视图（根须为 object/array/标量；仅 object 支持 [] 按键访问） */
    static JsonObjectView parse(const std::string& json) {
        JsonValue v;
        v.fromJson(json);
        return JsonObjectView(std::move(v));
    }

    JsonObjectView operator[](const std::string& key) const {
        if (node_.getType() != JsonValue::OBJECT)
            return JsonObjectView();
        const auto obj = node_.asObject();
        const auto it = obj.find(key);
        if (it == obj.end())
            return JsonObjectView();
        return JsonObjectView(it->second);
    }

    [[nodiscard]] const JsonValue& raw() const { return node_; }

    [[nodiscard]] JsonValue copy() const { return node_; }

    /** 与历史 jsonGetString 一致：string 返回原文；其它类型返回 toJson() */
    [[nodiscard]] std::string asString() const {
        if (node_.getType() == JsonValue::STRING)
            return node_.asString();
        return node_.toJson();
    }

    [[nodiscard]] int asInt(int defaultValue = 0) const {
        if (node_.getType() == JsonValue::NUMBER)
            return static_cast<int>(node_.asNumber());
        if (node_.getType() == JsonValue::STRING) {
            try {
                return std::stoi(node_.asString());
            }
            catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    [[nodiscard]] double asNumber(double defaultValue = 0.0) const {
        if (node_.getType() == JsonValue::NUMBER)
            return node_.asNumber();
        if (node_.getType() == JsonValue::STRING) {
            try {
                return std::stod(node_.asString());
            }
            catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    [[nodiscard]] bool isObject() const { return node_.getType() == JsonValue::OBJECT; }

    /** 当前为 object 时，浅拷贝一层键值到 unordered_map */
    [[nodiscard]] std::unordered_map<std::string, JsonValue> asUnorderedMap() const {
        std::unordered_map<std::string, JsonValue> out;
        if (node_.getType() != JsonValue::OBJECT)
            return out;
        for (const auto& [k, v] : node_.asObject())
            out.emplace(k, v);
        return out;
    }

private:
    JsonValue node_;
};

#endif // CBSF_JSON_OBJECT_VIEW_H
