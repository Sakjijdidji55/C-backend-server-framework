//
// Created by HP on 2025/12/16.
//

#ifndef CBSF_CONFIG_H
#define CBSF_CONFIG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <algorithm>
#include <regex>
#include <variant>
#include <optional>
#include <iomanip>

// YAML节点类型枚举
enum class YamlNodeType {
    SCALAR,
    SEQUENCE,
    MAPPING,
    NULL_VALUE,
    UNKNOWN
};

// YAML节点基类
class YamlNode {
public:
    YamlNode(YamlNodeType type) : type_(type) {}
    virtual ~YamlNode() = default;

    YamlNodeType getType() const { return type_; }
    virtual std::string toString() const = 0;
    virtual std::string toYamlString(int indent = 0) const = 0;

protected:
    YamlNodeType type_;
};

// 标量节点（支持多种数据类型）
class YamlScalarNode : public YamlNode {
public:
    YamlScalarNode(const std::string& value)
            : YamlNode(YamlNodeType::SCALAR), value_(value) {
        inferDataType();
    }

    // 获取原始字符串值
    std::string getString() const { return value_; }

    // 转换为各种类型
    template<typename T>
    T as() const {
        return convert<T>();
    }

    // 专门的类型转换方法
    int asInt(int defaultValue = 0) const {
        try {
            return std::stoi(value_);
        } catch (...) {
            return defaultValue;
        }
    }

    double asDouble(double defaultValue = 0.0) const {
        try {
            return std::stod(value_);
        } catch (...) {
            return defaultValue;
        }
    }

    bool asBool(bool defaultValue = false) const {
        std::string lower = value_;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "yes" || lower == "on" || lower == "1") {
            return true;
        } else if (lower == "false" || lower == "no" || lower == "off" || lower == "0") {
            return false;
        }
        return defaultValue;
    }

    std::string toString() const override {
        return value_;
    }

    std::string toYamlString(int indent = 0) const override {
        std::string result(indent, ' ');

        // 检查是否需要引号
        bool needsQuotes = false;
        if (value_.empty()) {
            return result + "\"\"";
        }

        // 包含特殊字符或看起来像其他类型的值需要引号
        if (value_.find_first_of(":{}[],&*!#|>\"'%@`") != std::string::npos ||
            std::isspace(value_.front()) || std::isspace(value_.back()) ||
            value_ == "null" || value_ == "Null" || value_ == "NULL" ||
            value_ == "true" || value_ == "false" ||
            value_ == "yes" || value_ == "no" ||
            value_ == "on" || value_ == "off") {
            needsQuotes = true;
        }

        if (needsQuotes) {
            // 处理转义字符
            std::string escaped = value_;
            size_t pos = 0;
            while ((pos = escaped.find('\"', pos)) != std::string::npos) {
                escaped.replace(pos, 1, "\\\"");
                pos += 2;
            }
            return result + "\"" + escaped + "\"";
        }

        return result + value_;
    }

private:
    std::string value_;

    // 推断数据类型
    void inferDataType() {
        // 这里可以添加更复杂的数据类型推断逻辑
    }

    // 类型转换模板
    template<typename T>
    T convert() const {
        if constexpr (std::is_same_v<T, int>) {
            return asInt();
        } else if constexpr (std::is_same_v<T, double>) {
            return asDouble();
        } else if constexpr (std::is_same_v<T, bool>) {
            return asBool();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return value_;
        } else {
            throw std::runtime_error("Unsupported type conversion");
        }
    }
};

// 序列节点（数组/列表）
class YamlSequenceNode : public YamlNode {
public:
    YamlSequenceNode() : YamlNode(YamlNodeType::SEQUENCE) {}

    void addNode(std::shared_ptr<YamlNode> node) {
        items_.push_back(node);
    }

    size_t size() const { return items_.size(); }

    std::shared_ptr<YamlNode> get(size_t index) const {
        if (index >= items_.size()) {
            return nullptr;
        }
        return items_[index];
    }

    const std::vector<std::shared_ptr<YamlNode>>& getItems() const {
        return items_;
    }

    std::string toString() const override {
        std::string result = "[";
        for (size_t i = 0; i < items_.size(); ++i) {
            if (i > 0) result += ", ";
            result += items_[i]->toString();
        }
        result += "]";
        return result;
    }

    std::string toYamlString(int indent = 0) const override {
        std::string result;
        if (items_.empty()) {
            result = std::string(indent, ' ') + "[]\n";
        } else {
            for (const auto& item : items_) {
                result += std::string(indent, ' ') + "- " +
                          item->toYamlString(0).substr(indent + 2) + "\n";
            }
        }
        return result;
    }

private:
    std::vector<std::shared_ptr<YamlNode>> items_;
};

// 映射节点（键值对字典）
class YamlMappingNode : public YamlNode {
public:
    YamlMappingNode() : YamlNode(YamlNodeType::MAPPING) {}

    void addNode(const std::string& key, std::shared_ptr<YamlNode> node) {
        items_[key] = node;
    }

    bool hasKey(const std::string& key) const {
        return items_.find(key) != items_.end();
    }

    std::shared_ptr<YamlNode> get(const std::string& key) const {
        auto it = items_.find(key);
        return (it != items_.end()) ? it->second : nullptr;
    }

    const std::map<std::string, std::shared_ptr<YamlNode>>& getItems() const {
        return items_;
    }

    std::string toString() const override {
        std::string result = "{";
        bool first = true;
        for (const auto& [key, node] : items_) {
            if (!first) result += ", ";
            result += key + ": " + node->toString();
            first = false;
        }
        result += "}";
        return result;
    }

    std::string toYamlString(int indent = 0) const override {
        std::string result;
        for (const auto& [key, node] : items_) {
            std::string keyStr = key;
            // 如果键包含特殊字符，需要加引号
            if (key.find_first_of(":[]{}#&*!|>\"'%@`") != std::string::npos ||
                std::isspace(key.front()) || std::isspace(key.back())) {
                keyStr = "\"" + key + "\"";
            }

            result += std::string(indent, ' ') + keyStr + ":";

            if (node->getType() == YamlNodeType::SCALAR) {
                std::string nodeStr = node->toYamlString(0);
                // 去除缩进空格
                nodeStr.erase(0, nodeStr.find_first_not_of(' '));
                result += " " + nodeStr + "\n";
            } else {
                result += "\n" + node->toYamlString(indent + 2);
            }
        }
        return result;
    }

private:
    std::map<std::string, std::shared_ptr<YamlNode>> items_;
};

// 增强版YAML解析器
class YamlParser {
public:
    YamlParser() = default;
    explicit YamlParser(const std::string& configPath) : configPath_(configPath) {}

    /**
     * 加载并解析YAML文件
     * @return 解析成功返回true，失败返回false
     */
    bool loadYaml() {
        std::ifstream file(configPath_);
        if (!file.is_open()) {
            std::cerr << "[YamlParser] Failed to open file: " << configPath_ << std::endl;
            return false;
        }

        // 读取所有行
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();

        // 解析为根节点
        size_t pos = 0;
        try {
            rootNode_ = parseDocument(lines, pos);
            return rootNode_ != nullptr;
        } catch (const std::exception& e) {
            std::cerr << "[YamlParser] Parse error: " << e.what() << " at line " << pos + 1 << std::endl;
            return false;
        }
    }

    /**
     * 获取根节点
     */
    std::shared_ptr<YamlNode> getRoot() const {
        return rootNode_;
    }

    /**
     * 通过路径获取节点（支持点分隔和数组索引）
     * @param path 路径，如 "database.mysql.hosts[0]"
     * @return 节点指针，不存在返回nullptr
     */
    std::shared_ptr<YamlNode> getNode(const std::string& path) const {
        if (!rootNode_) return nullptr;

        std::shared_ptr<YamlNode> current = rootNode_;
        std::string token;
        std::istringstream stream(path);

        while (std::getline(stream, token, '.')) {
            if (current->getType() != YamlNodeType::MAPPING) {
                return nullptr;
            }

            // 检查是否包含数组索引
            size_t bracketPos = token.find('[');
            if (bracketPos != std::string::npos) {
                // 提取键名
                std::string key = token.substr(0, bracketPos);
                auto mapping = std::dynamic_pointer_cast<YamlMappingNode>(current);
                current = mapping->get(key);

                if (!current || current->getType() != YamlNodeType::SEQUENCE) {
                    return nullptr;
                }

                // 提取索引
                size_t endBracketPos = token.find(']', bracketPos);
                if (endBracketPos == std::string::npos) {
                    return nullptr;
                }

                std::string indexStr = token.substr(bracketPos + 1, endBracketPos - bracketPos - 1);
                try {
                    size_t index = std::stoul(indexStr);
                    auto sequence = std::dynamic_pointer_cast<YamlSequenceNode>(current);
                    current = sequence->get(index);
                } catch (...) {
                    return nullptr;
                }
            } else {
                auto mapping = std::dynamic_pointer_cast<YamlMappingNode>(current);
                current = mapping->get(token);
            }

            if (!current) {
                return nullptr;
            }
        }

        return current;
    }

    /**
     * 获取标量值
     * @param path 路径
     * @param defaultValue 默认值
     * @return 字符串值
     */
    std::string getString(const std::string& path, const std::string& defaultValue = "") const {
        auto node = getNode(path);
        if (!node || node->getType() != YamlNodeType::SCALAR) {
            return defaultValue;
        }
        auto scalar = std::dynamic_pointer_cast<YamlScalarNode>(node);
        return scalar->getString();
    }

    /**
     * 获取整数
     */
    int getInt(const std::string& path, int defaultValue = 0) const {
        auto node = getNode(path);
        if (!node || node->getType() != YamlNodeType::SCALAR) {
            return defaultValue;
        }
        auto scalar = std::dynamic_pointer_cast<YamlScalarNode>(node);
        return scalar->asInt(defaultValue);
    }

    /**
     * 获取浮点数
     */
    double getDouble(const std::string& path, double defaultValue = 0.0) const {
        auto node = getNode(path);
        if (!node || node->getType() != YamlNodeType::SCALAR) {
            return defaultValue;
        }
        auto scalar = std::dynamic_pointer_cast<YamlScalarNode>(node);
        return scalar->asDouble(defaultValue);
    }

    /**
     * 获取布尔值
     */
    bool getBool(const std::string& path, bool defaultValue = false) const {
        auto node = getNode(path);
        if (!node || node->getType() != YamlNodeType::SCALAR) {
            return defaultValue;
        }
        auto scalar = std::dynamic_pointer_cast<YamlScalarNode>(node);
        return scalar->asBool(defaultValue);
    }

    /**
     * 获取数组
     */
    std::vector<std::string> getStringArray(const std::string& path) const {
        std::vector<std::string> result;
        auto node = getNode(path);
        if (!node || node->getType() != YamlNodeType::SEQUENCE) {
            return result;
        }

        auto sequence = std::dynamic_pointer_cast<YamlSequenceNode>(node);
        for (size_t i = 0; i < sequence->size(); ++i) {
            auto item = sequence->get(i);
            if (item->getType() == YamlNodeType::SCALAR) {
                auto scalar = std::dynamic_pointer_cast<YamlScalarNode>(item);
                result.push_back(scalar->getString());
            }
        }
        return result;
    }

    /**
     * 设置配置路径
     */
    void setConfigPath(const std::string& path) {
        configPath_ = path;
    }

    /**
     * 保存YAML到文件
     */
    bool saveToFile(const std::string& path = "") {
        std::string savePath = path.empty() ? configPath_ : path;
        if (!rootNode_) {
            return false;
        }

        std::ofstream file(savePath);
        if (!file.is_open()) {
            return false;
        }

        file << rootNode_->toYamlString();
        file.close();
        return true;
    }

    /**
     * 打印YAML结构（调试用）
     */
    void printYaml() const {
        if (rootNode_) {
            std::cout << rootNode_->toYamlString();
        }
    }

private:
    // 解析整个文档
    std::shared_ptr<YamlNode> parseDocument(const std::vector<std::string>& lines, size_t& pos) {
        while (pos < lines.size()) {
            std::string line = trim(lines[pos]);

            // 跳过空行和注释
            if (line.empty() || line[0] == '#') {
                pos++;
                continue;
            }

            // 解析YAML开始（可能是映射或序列）
            return parseNode(lines, pos, 0);
        }

        return std::make_shared<YamlMappingNode>(); // 返回空映射
    }

    // 解析节点
    std::shared_ptr<YamlNode> parseNode(const std::vector<std::string>& lines, size_t& pos, int indent) {
        if (pos >= lines.size()) {
            return nullptr;
        }

        std::string line = lines[pos];
        int currentIndent = getIndentLevel(line);

        if (currentIndent < indent) {
            return nullptr; // 返回上层
        }

        line = trim(line);

        // 检查是否为序列项
        if (line.size() >= 2 && line[0] == '-') {
            return parseSequence(lines, pos, indent);
        }

        // 检查是否为映射
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            return parseMapping(lines, pos, indent);
        }

        // 否则是标量
        return std::make_shared<YamlScalarNode>(line);
    }

    // 解析序列
    std::shared_ptr<YamlNode> parseSequence(const std::vector<std::string>& lines, size_t& pos, int indent) {
        auto sequence = std::make_shared<YamlSequenceNode>();

        while (pos < lines.size()) {
            std::string line = lines[pos];
            int currentIndent = getIndentLevel(line);

            if (currentIndent < indent) {
                break; // 返回到上层
            }

            line = trim(line);

            if (line.empty() || line[0] == '#') {
                pos++;
                continue;
            }

            if (line[0] == '-') {
                // 序列项
                std::string itemValue = trim(line.substr(1));

                if (itemValue.empty()) {
                    // 空项，可能是嵌套结构
                    pos++;
                    auto nestedNode = parseNode(lines, pos, currentIndent + 2);
                    if (nestedNode) {
                        sequence->addNode(nestedNode);
                    }
                    continue;
                }

                // 检查项值是否以冒号结尾（可能是嵌套映射）
                if (itemValue.back() == ':') {
                    itemValue.pop_back();
                    sequence->addNode(std::make_shared<YamlScalarNode>(itemValue));
                    pos++;
                    auto nestedMapping = parseMapping(lines, pos, currentIndent + 2);
                    if (nestedMapping) {
                        sequence->addNode(nestedMapping);
                    }
                } else {
                    sequence->addNode(std::make_shared<YamlScalarNode>(itemValue));
                    pos++;
                }
            } else {
                // 应该是缩进错误的项，尝试继续
                auto node = parseNode(lines, pos, indent);
                if (node) {
                    sequence->addNode(node);
                } else {
                    pos++;
                }
            }
        }

        return sequence;
    }

    // 解析映射
    std::shared_ptr<YamlNode> parseMapping(const std::vector<std::string>& lines, size_t& pos, int indent) {
        auto mapping = std::make_shared<YamlMappingNode>();

        while (pos < lines.size()) {
            std::string line = lines[pos];
            int currentIndent = getIndentLevel(line);

            if (currentIndent < indent) {
                break; // 返回到上层
            }

            line = trim(line);

            if (line.empty() || line[0] == '#') {
                pos++;
                continue;
            }

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                // 不是键值对，可能是序列项或其他
                pos++;
                continue;
            }

            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));

            // 去除键的引号
            if (!key.empty() && ((key.front() == '\"' && key.back() == '\"') ||
                                 (key.front() == '\'' && key.back() == '\''))) {
                key = key.substr(1, key.size() - 2);
            }

            if (value.empty()) {
                // 值是空的，可能是嵌套结构
                pos++;
                auto nestedNode = parseNode(lines, pos, currentIndent + 2);
                if (nestedNode) {
                    mapping->addNode(key, nestedNode);
                }
            } else if (value == "|") {
                // 多行字符串（文字块）
                pos++;
                std::string multiLineValue = parseMultilineString(lines, pos, currentIndent + 2);
                mapping->addNode(key, std::make_shared<YamlScalarNode>(multiLineValue));
            } else if (value == ">") {
                // 折叠多行字符串
                pos++;
                std::string foldedValue = parseFoldedString(lines, pos, currentIndent + 2);
                mapping->addNode(key, std::make_shared<YamlScalarNode>(foldedValue));
            } else {
                // 简单标量值
                // 去除值的引号
                if (!value.empty() && ((value.front() == '\"' && value.back() == '\"') ||
                                       (value.front() == '\'' && value.back() == '\''))) {
                    value = value.substr(1, value.size() - 2);
                }
                mapping->addNode(key, std::make_shared<YamlScalarNode>(value));
                pos++;
            }
        }

        return mapping;
    }

    // 解析多行字符串（文字块）
    std::string parseMultilineString(const std::vector<std::string>& lines, size_t& pos, int indent) {
        std::string result;

        while (pos < lines.size()) {
            std::string line = lines[pos];
            int currentIndent = getIndentLevel(line);

            if (currentIndent < indent) {
                break;
            }

            line = line.substr(currentIndent);
            result += line + "\n";
            pos++;
        }

        // 移除最后一个换行符
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }

        return result;
    }

    // 解析折叠多行字符串
    std::string parseFoldedString(const std::vector<std::string>& lines, size_t& pos, int indent) {
        std::string result;

        while (pos < lines.size()) {
            std::string line = lines[pos];
            int currentIndent = getIndentLevel(line);

            if (currentIndent < indent) {
                break;
            }

            line = trim(line.substr(currentIndent));
            if (!result.empty() && !line.empty()) {
                result += " ";
            }
            result += line;
            pos++;
        }

        return result;
    }

    // 辅助函数
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t");
        size_t end = str.find_last_not_of(" \t");
        return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }

    static int getIndentLevel(const std::string& line) {
        size_t spaceCount = 0;
        while (spaceCount < line.size() && line[spaceCount] == ' ') {
            spaceCount++;
        }
        return static_cast<int>(spaceCount);
    }

private:
    std::string configPath_;
    std::shared_ptr<YamlNode> rootNode_;
};
#endif //CBSF_CONFIG_H
