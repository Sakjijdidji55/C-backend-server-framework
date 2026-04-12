//
// Created by HP on 2026/4/3.
//

#include "YamlParser.h"
#include "Config.h"

#include <iostream>
#include <utility>

Config* Config::instance = nullptr;

namespace {

void read(Config* config, const YamlParser& parser) {
    config->server.port = parser.getInt("server.port", 8080);

    config->mysql.enabled = parser.getBool("mysql.enabled", true);
    config->mysql.port = parser.getInt("mysql.port", 3306);
    config->mysql.host = parser.getString("mysql.host", "127.0.0.1");
    config->mysql.user = parser.getString("mysql.user", "root");
    config->mysql.password = parser.getString("mysql.password", "");
    config->mysql.database = parser.getString("mysql.database", "airi");

    config->redis.enabled = parser.getBool("redis.enabled", true);
    config->redis.port = parser.getInt("redis.port", 6379);
    config->redis.host = parser.getString("redis.host", "127.0.0.1");
    config->redis.password = parser.getString("redis.password", "");
    config->redis.db = parser.getInt("redis.db", 0);

    config->smtp.enabled = parser.getBool("smtp.enabled", false);
    config->smtp.port = parser.getInt("smtp.port", 587);
    config->smtp.host = parser.getString("smtp.host", "");
    config->smtp.user = parser.getString("smtp.username", "");
    config->smtp.password = parser.getString("smtp.password", "");
    config->smtp.from = parser.getString("smtp.from", "");

    config->auth.code_length = parser.getInt("auth.code_length", 6);
    config->auth.code_ttl_seconds = parser.getInt("auth.code_ttl_seconds", 600);
    config->auth.send_rate_limit_seconds = parser.getInt("auth.send_rate_limit_seconds", 60);

    config->jwt.secret = parser.getString("jwt.secret", "");
//    std::cout<< "jwt.secret: " << parser.getString("mysql.password", "") << std::endl;
    config->jwt.ttl_seconds = parser.getInt("jwt.ttl_seconds", 60 * 60 * 24 * 7);

    config->oauth.enabled = parser.getBool("oauth_connect.enabled", false);
//    std::cout<< "oauth_connect.enabled: " << config->oauth.enabled << std::endl;
    config->oauth.app_id = parser.getString("oauth_connect.app_id", "");
    config->oauth.app_secret = parser.getString("oauth_connect.app_secret", "");
    config->oauth.base_url = parser.getString("oauth_connect.base_url", "");
    config->oauth.scope = parser.getString("oauth_connect.scope", "");

    config->llm_proxy.enabled = parser.getBool("llm_proxy.enabled", false);
    config->llm_proxy.upstream_base_url = parser.getString("llm_proxy.upstream_base_url", "");
    config->llm_proxy.chat_completions_path = parser.getString("llm_proxy.chat_completions_path", "/v1/chat/completions");
    config->llm_proxy.api_key = parser.getString("llm_proxy.api_key", "");
    config->llm_proxy.timeout_ms = parser.getInt("llm_proxy.timeout_ms", 60000);
}

} // namespace

Config::Config(std::string configPath) {
    if (instance) {
        std::cerr << "[config] Config singleton already initialized\n";
        exit(1);
    }
    path = configPath.empty() ? "config.yml" : std::move(configPath);

    YamlParser parser(path);
    if (!parser.loadYaml()) {
        std::cerr << "Failed to parse config file: " << path << std::endl;
        exit(1);
    }

    read(this, parser);
    instance = this;
}

Config* Config::getInstance(const std::string& configPath) {
    if (!instance)
        new Config(configPath);
    return instance;
}
