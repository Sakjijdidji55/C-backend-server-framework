/**
 * @file EmailSender.h
 * @brief 邮件发送类头文件（单例模式）
 * @brief Email Sender Class Header (Singleton Pattern)
 */
#ifndef CBSF_EMAILSENDER_H
#define CBSF_EMAILSENDER_H

#include <string>
#include <vector>
#include <mutex>

/**
 * @class EmailSender
 * @brief 邮件发送类，单例模式，用于通过 SMTP 发送邮件
 * @note 提供初始化 SMTP 配置与发送邮件方法，线程安全
 */
class EmailSender {
public:
    /**
     * @brief 获取单例实例
     * @return EmailSender 实例指针
     */
    static EmailSender* getInstance();

    /**
     * @brief 初始化 SMTP 配置（在首次发送前调用）
     * @param smtpHost SMTP 服务器地址，如 smtp.qq.com
     * @param smtpPort SMTP 端口，如 25 / 465 / 587
     * @param username 登录用户名（一般为邮箱）
     * @param password 登录密码或授权码
     */
    void init(const std::string& smtpHost, unsigned short smtpPort,
              const std::string& username, const std::string& password);

    /**
     * @brief 发送一封邮件
     * @param from 发件人邮箱
     * @param to 收件人邮箱（多个用逗号分隔）
     * @param subject 邮件主题
     * @param body 邮件正文（纯文本）
     * @return 发送成功返回 true，失败返回 false
     */
    bool sendMail(const std::string& from, const std::string& to,
                  const std::string& subject, const std::string& body);

    /**
     * @brief 发送一封邮件（支持多个收件人）
     * @param from 发件人邮箱
     * @param toList 收件人邮箱列表
     * @param subject 邮件主题
     * @param body 邮件正文（纯文本）
     * @return 发送成功返回 true，失败返回 false
     */
    bool sendMail(const std::string& from, const std::vector<std::string>& toList,
                  const std::string& subject, const std::string& body);

    /**
     * @brief 获取最后一次错误信息
     */
    [[nodiscard]] std::string getLastError() const { return lastError_; }

    EmailSender(const EmailSender&) = delete;
    EmailSender& operator=(const EmailSender&) = delete;

private:
    EmailSender() = default;
    ~EmailSender() = default;

    bool sendMailImpl(const std::string& from, const std::vector<std::string>& toList,
                      const std::string& subject, const std::string& body);
    bool smtpConnect(int* sockOut);
    bool smtpCommand(int sock, const std::string& cmd, int expectCode);
    bool smtpCommandRecv(int sock, std::string* outLine);
    static std::string base64Encode(const std::string& raw);

    std::string smtpHost_;
    unsigned short smtpPort_ = 25;
    std::string username_;
    std::string password_;
    std::string lastError_;
    std::mutex mutex_;
    static EmailSender* instance_;
};

#endif // CBSF_EMAILSENDER_H
