/**
 * @file ApiSender.h
 * @brief 外部 API 请求类头文件（异步、回调、多请求方法）
 * @brief External API Sender Class Header (Async, Callback, Multiple HTTP Methods)
 */
#ifndef CBSF_APISENDER_H
#define CBSF_APISENDER_H

#include <string>
#include <map>
#include <functional>
#include <memory>
#include "Threadpool.h"

/**
 * @brief 一次 API 请求的响应结果
 */
struct ApiResponse {
    int statusCode = 0;           ///< HTTP 状态码，0 表示未收到或出错
    std::string body;             ///< 响应体
    std::string error;            ///< 错误信息（网络/解析错误等）
    bool success = false;         ///< 是否成功收到 HTTP 响应
};

/**
 * @brief 异步请求完成时的回调类型
 * @param resp 响应结果（statusCode、body、error、success）
 */
using ApiCallback = std::function<void(const ApiResponse& resp)>;

/**
 * @enum HttpMethod
 * @brief HTTP 请求方法
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

/**
 * @class ApiSender
 * @brief 用于调用外部公司提供的 HTTP API，支持异步、回调、多种请求方法
 * @note 内部使用线程池执行请求，回调在池内线程中执行
 */
class ApiSender {
public:
    /**
     * @brief 构造函数，可指定异步工作线程数
     * @param numWorkers 异步请求使用的线程数，默认 4
     */
    explicit ApiSender(size_t numWorkers = 4);

    ~ApiSender();

    ApiSender(const ApiSender&) = delete;
    ApiSender& operator=(const ApiSender&) = delete;

    /**
     * @brief 通用请求（异步）
     * @param method HTTP 方法
     * @param url 完整 URL，如 http://api.example.com/v1/foo
     * @param headers 请求头（可选）
     * @param body 请求体（POST/PUT/PATCH 时使用）
     * @param callback 请求完成后的回调
     */
    void request(HttpMethod method, const std::string& url,
                 const std::map<std::string, std::string>& headers,
                 const std::string& body, ApiCallback callback);

    /**
     * @brief GET 请求（异步）
     */
    void get(const std::string& url,
             const std::map<std::string, std::string>& headers,
             ApiCallback callback);

    /**
     * @brief POST 请求（异步）
     */
    void post(const std::string& url,
              const std::map<std::string, std::string>& headers,
              const std::string& body, ApiCallback callback);

    /**
     * @brief PUT 请求（异步）
     */
    void put(const std::string& url,
             const std::map<std::string, std::string>& headers,
             const std::string& body, ApiCallback callback);

    /**
     * @brief DELETE 请求（异步）
     */
    void del(const std::string& url,
             const std::map<std::string, std::string>& headers,
             ApiCallback callback);

    /**
     * @brief PATCH 请求（异步）
     */
    void patch(const std::string& url,
               const std::map<std::string, std::string>& headers,
               const std::string& body, ApiCallback callback);

    /**
     * @brief 同步请求（阻塞当前线程，适用于简单场景）
     * @return 响应结果
     */
    ApiResponse requestSync(HttpMethod method, const std::string& url,
                            const std::map<std::string, std::string>& headers = {},
                            const std::string& body = "");

private:
    static std::string methodToString(HttpMethod method);
    static ApiResponse doRequest(HttpMethod method, const std::string& url,
                                 const std::map<std::string, std::string>& headers,
                                 const std::string& body);

    std::unique_ptr<ThreadPool> pool_;
};

#endif // CBSF_APISENDER_H
