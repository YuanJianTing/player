#include "http_client.h"
#include <httplib.h>
#include <sstream>
#include <json/json.h>

HttpClient::HttpClient(const std::string &url_root) : url_root_(url_root)
{
    // 解析URL获取主机和端口
    size_t protocol_pos = url_root.find("://");
    if (protocol_pos == std::string::npos)
    {
        throw std::runtime_error("Invalid URL format");
    }

    std::string protocol = url_root.substr(0, protocol_pos);
    std::string host_port_path = url_root.substr(protocol_pos + 3);

    size_t slash_pos = host_port_path.find('/');
    std::string host_port = host_port_path.substr(0, slash_pos);

    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos)
    {
        std::string host_ = host_port.substr(0, colon_pos);
        int port_ = std::stoi(host_port.substr(colon_pos + 1));
    }
    else
    {
        std::string host_ = host_port;
        int port_ = (protocol == "https") ? 443 : 80;
    }

    is_https_ = (protocol == "https");
}

bool HttpClient::getSystemInfo(SystemInfo &info, std::string &error_msg)
{
    std::string path = "api/system/systemInfo";
    std::string response;

    if (!performGetRequest(path, response, error_msg))
    {
        return false;
    }

    try
    {
        Json::Value json;
        Json::Reader reader;
        bool parsingSuccessful = reader.parse(response, json);
        if (!parsingSuccessful)
        {
            error_msg = "JSON解析失败";
            return false;
        }

        if (json["code"].asInt() != 0)
        {
            error_msg = json["message"].asString();
            return false;
        }

        auto body = json["body"];
        info.language = body["language"].asString();
        info.appName = body["appName"].asString();
        info.customerCode = body["customerCode"].asString();
        info.customerName = body["customerName"].asString();
        info.host = body["host"].asString();
        info.mqtt = body["mqtt"].asString();
        info.ntpServer = body["ntpServer"].asString();

        return true;
    }
    catch (const std::exception &e)
    {
        error_msg = "JSON解析失败: " + std::string(e.what());
        return false;
    }
}

bool HttpClient::performGetRequest(const std::string &path, std::string &response, std::string &error_msg)
{
    try
    {
        // std::cout << "http:" << url_root_ << std::endl;
        httplib::Client client(url_root_); // scheme + host

        // 设置超时（单位：秒）
        client.set_connection_timeout(10);
        client.set_read_timeout(10);
        client.set_write_timeout(10);

        // 如果是HTTPS，需要启用SSL
        if (is_https_)
        {
            client.enable_server_certificate_verification(false); // 禁用证书验证（生产环境应启用）
        }

        auto res = client.Get(path.c_str());

        if (!res)
        {
            error_msg = "HTTP请求失败: 无法连接到服务器";
            return false;
        }
        if (res->status != 200)
        {
            error_msg = "HTTP请求失败: 状态码 " + std::to_string(res->status);
            return false;
        }

        response = res->body;
        return true;
    }
    catch (const std::exception &e)
    {
        error_msg = "HTTP请求异常: " + std::string(e.what());
        return false;
    }
}