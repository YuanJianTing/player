#include <control.h>
#include <string>
#include <http_client.h>
#include <iostream>
#include <mqtt_client.h>
#include <downloader.h>


Control::Control(const std::string& url_root, const std::string& client_id)
    : url_root_(url_root),
      client_id_(client_id),
      downloader_(url_root)
{
}

Control::~Control() {
    if (mqtt_client_ && mqtt_client_->isConnected()) {
        mqtt_client_->disconnect();
    }
}

void Control::start(){
    
    //获取系统参数
    HttpClient client(url_root_);

    HttpClient::SystemInfo info;
    std::string error;

    if (client.getSystemInfo(info, error)) {
        std::cout << "系统信息获取成功:" << std::endl;
        std::cout << "应用名称: " << info.appName << std::endl;
        std::cout << "客户名称: " << info.customerName << std::endl;
        std::cout << "MQTT地址: " << info.mqtt << std::endl;
    } else {
        std::cerr << "获取配置信息错误: " << error << std::endl;
        return;
    }

    //链接mqtt服务器
    mqtt_client_ = std::make_shared<MQTTClient>("tcp://"+info.mqtt, client_id_, "LCD", "eTagTech@Pass");
    // 设置回调函数
    mqtt_client_->setMessageCallback([this](const std::string& code, const std::string& body) {
        this->handleMessage(code, body);
    });

     // 确保连接成功
    if (!mqtt_client_->isConnected()) {
        std::cerr << "MQTT连接失败" << std::endl;
        return;
    }
}

//处理接受到的消息
void Control::handleMessage(const std::string& code, const std::string& body){
    std::cout << "Received message - Code: " << code << ", Body: " << body << std::endl;

    if("0001"==code){
        //处理心跳反馈
        if(body.empty()||body.size()==0)return;

         if(daemon_thread_){
            daemon_thread_->updateTask(body);
         }
    }else if("0002"==code){
        //处理播放列表
        printf("Received task, start caching");
        
    }else if("0005"==code){
        //设置屏幕亮度
        printf("设置屏幕亮度:%s",body);
    }else if("0006"==code){
        printf("关闭屏幕");
    }else if("0008"==code){
        //处理配置信息  200&1748419573519&60&&
         printf("处理配置信息:%s",body);
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(body);
        // 按'&'分割字符串
        while (std::getline(tokenStream, token, '&')) {
            tokens.push_back(token);
        }
        // 确保有足够的分割部分
        if (tokens.size() >= 5) {
            try {
                int brightness = std::stoi(tokens[0]);// 屏幕亮度
                int64_t time = std::stoll(tokens[1]);// 服务器时间戳
                int speed = std::stoi(tokens[2]);// 心跳间隔
                int64_t openTime = tokens[3].empty() ? 0 : std::stoll(tokens[3]);// 系统开机时间戳
                int64_t closeTime = tokens[4].empty() ? 0 : std::stoll(tokens[4]);// 系统关机时间戳

                heartbeat(speed);

            } catch (const std::exception& e) {
                std::cerr << "配置参数解析错误: " << e.what() << std::endl;
            }
        } else {
             std::cerr << "配置参数格式无效 " <<  std::endl;
        }
    }

}

void Control::heartbeat(const std::int32_t& speed){
    if(daemon_thread_){
        daemon_thread_->stop();
    }
    //std::shared_ptr<MQTTClient> mqtt_client_;
    daemon_thread_ = std::make_shared<DaemonThread>(mqtt_client_,speed);
    daemon_thread_->start();
}

void Control::reset_config(){
    // 实现重置配置的逻辑
    if (mqtt_client_) {
        mqtt_client_->disconnect();
    }
}