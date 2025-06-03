#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <memory>
#include <json/json.h>

class HttpClient
{
public:
    // 系统信息结构体
    struct SystemInfo
    {
        std::string language;
        std::string appName;
        std::string customerCode;
        std::string customerName;
        std::string host;
        std::string mqtt;
        std::string ntpServer;
    };

    // 构造函数
    HttpClient(const std::string &url_root);

    // 获取系统信息
    bool getSystemInfo(SystemInfo &info, std::string &error_msg);

private:
    // 执行HTTP GET请求
    std::string performGetRequest(const std::string &url, std::string &error_msg);

    std::string url_root_;
};
#endif // HTTP_CLIENT_H