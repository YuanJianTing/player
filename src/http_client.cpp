#include "http_client.h"
#include <curl/curl.h>
#include <sstream>

// 回调函数用于接收HTTP响应数据
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

HttpClient::HttpClient(const std::string& url_root) : url_root_(url_root) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

bool HttpClient::getSystemInfo(SystemInfo& info, std::string& error_msg) {
    std::string url = url_root_ + "api/system/systemInfo";
    std::string response;
    
    response = performGetRequest(url, error_msg);
    if (error_msg.empty()) {
        try {
            auto json = nlohmann::json::parse(response);
            
            if (json["code"] != 0) {
                error_msg = json["message"];
                return false;
            }

            auto body = json["body"];
            info.language = body["language"];
            info.appName = body["appName"];
            info.customerCode = body["customerCode"];
            info.customerName = body["customerName"];
            info.host = body["host"];
            info.mqtt = body["mqtt"];
            info.ntpServer = body["ntpServer"];
            
            return true;
        } catch (const std::exception& e) {
            error_msg = "JSON解析失败: " + std::string(e.what());
            return false;
        }
    }
    return false;
}

std::string HttpClient::performGetRequest(const std::string& url, std::string& error_msg) {
    CURL* curl = curl_easy_init();
    std::string response_data;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            error_msg = "HTTP请求失败: " + std::string(curl_easy_strerror(res));
        }
        
        curl_easy_cleanup(curl);
    } else {
        error_msg = "无法初始化cURL";
    }
    
    return response_data;
}