//
// Created by HP on 2025/12/16.
//

#ifndef CBSF_CONFIG_H
#define CBSF_CONFIG_H

#include <string>

struct ServerConfig {
    int port = 0;
};

struct MysqlConfig {
    bool enabled = false;
    std::string host;
    int port = 0;
    std::string user;
    std::string password;
    std::string database;
};

struct RedisConfig {
    bool enabled = false;
    std::string host;
    int port = 0;
    std::string password;
    int db = 0;
};

struct JwtConfig {
    std::string secret;
    long ttl_seconds = 0;
};

struct StmpConfig {
    bool enabled = false;
    std::string host;
    int port = 0;
    std::string user;
    std::string password;
    std::string from;
};

struct AuthConfig {
    long code_length = 0;
    long code_ttl_seconds = 0;
    long send_rate_limit_seconds = 0;
};

struct OauthConfig {
    bool enabled = false;
    std::string base_url;
    std::string app_id;
    std::string app_secret;
    std::string scope;
};

/** 与 config.yml 中 llm_proxy.* 一致（见 handlers/llm_proxy_handler.cpp） */
struct LlmProxyConfig {
    bool enabled = false;
    std::string upstream_base_url;
    std::string chat_completions_path = "/v1/chat/completions";
    std::string api_key;
    int timeout_ms = 60000;
};

class Config {
public:
    ServerConfig server;
    MysqlConfig mysql;
    RedisConfig redis;
    JwtConfig jwt;
    StmpConfig smtp;
    AuthConfig auth;
    OauthConfig oauth;
    LlmProxyConfig llm_proxy;

    static Config* instance;

    std::string path;

    /** 从 YAML 加载；path 为空时使用 config.yml */
    explicit Config(std::string configPath = "config.yml");

    /** 首次调用时创建单例并加载；可传入与 main 一致的配置文件路径 */
    static Config* getInstance(const std::string& configPath = "config.yml");
};

#endif //CBSF_CONFIG_H
