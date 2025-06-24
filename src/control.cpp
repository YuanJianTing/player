#include "control.h"
#include <string>
#include <http_client.h>
#include <iostream>
#include <mqtt_client.h>
#include <downloader.h>
#include "task_repository.h"
#include <logger.h>

Control::Control(const std::string &url_root, const std::string &client_id)
    : url_root_(url_root),
      client_id_(client_id),
      downloader_(url_root),
      task_repository_()
{
    // 查找显示器设备 ls /dev/fb*
    // 使用 , 号分割 client_id
    size_t comma_pos = client_id.find(',');
    if (comma_pos > 0)
    {
        std::string client_a = client_id.substr(0, comma_pos);
        std::string client_b = client_id.substr(comma_pos + 1);

        display_ = std::make_shared<Display>(client_a, "/dev/fb0"); // 初始化显示器
    }
    else
    {
        display_ = std::make_shared<Display>(client_id, "/dev/fb1"); // 初始化显示器
    }
    downloader_.setDownloadCallback([this](const MediaItem &media, const std::string &local_path, bool success, const std::string &error)
                                    { this->downloadCallback(media, local_path, success, error); });
}

Control::~Control()
{
    if (mqtt_client_ && mqtt_client_->isConnected())
    {
        mqtt_client_->disconnect();
    }

    if (display_)
    {
        display_->~Display();
    }
}

void Control::show()
{
    display_->show_info();
    refresh(display_->getDeviceId());
}

void Control::start()
{
    // 获取系统参数
    HttpClient client(url_root_);

    HttpClient::SystemInfo info;
    std::string error;

    if (client.getSystemInfo(info, error))
    {
        LOGI("Control", "系统信息获取成功:");
        LOGI("Control", "应用名称:%s", info.appName.c_str());
        LOGI("Control", "客户名称:%s", info.customerName.c_str());
        LOGI("Control", "MQTT地址:%s", info.mqtt.c_str());
    }
    else
    {
        LOGE("Control", "获取配置信息错误:%s", error.c_str());
        return;
    }

    // 链接mqtt服务器
    mqtt_client_ = std::make_shared<mqtt_client>("tcp://" + info.mqtt, client_id_, "LCD", "eTagTech@Pass");
    // 设置回调函数
    mqtt_client_->setMessageCallback([this](const std::string &code, const std::string &body)
                                     { this->handleMessage(code, body); });

    // 确保连接成功
    if (!mqtt_client_->isConnected())
    {
        LOGE("Control", "MQTT连接失败");
        return;
    }
}

// 处理接受到的消息
void Control::handleMessage(const std::string &code, const std::string &body)
{
    LOGI("Control", "Received message - Code:%d ,Body: %s ", code, body.c_str());

    if ("0001" == code)
    {
        // 处理心跳反馈
        if (body.empty() || body.size() == 0)
            return;

        if (daemon_thread_)
        {
            daemon_thread_.get()->updateTask(body);
        }
    }
    else if ("0002" == code)
    {
        // 处理播放列表
        LOGI("Control", "Received task, start caching ");
        std::string device_id = task_repository_.saveTask(body);
        if (device_id.empty())
            return;
        this->refresh(device_id);
    }
    else if ("0005" == code)
    {
        // 设置屏幕亮度
        std::cerr << "设置屏幕亮度:" << body << std::endl;
    }
    else if ("0006" == code)
    {
        std::cerr << "关闭屏幕:" << std::endl;
    }
    else if ("0008" == code)
    {
        // 处理配置信息  200&1748419573519&60&&
        // std::cerr << "处理配置信息:" << body << std::endl;
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(body);
        // 按'&'分割字符串
        while (std::getline(tokenStream, token, '&'))
        {
            tokens.push_back(token);
        }
        // std::cerr << "参数长度:" << tokens.size() << std::endl;
        //  确保有足够的分割部分
        if (tokens.size() >= 3)
        {
            try
            {
                int brightness = std::stoi(tokens[0]); // 屏幕亮度
                int64_t time = std::stoll(tokens[1]);  // 服务器时间戳
                int speed = std::stoi(tokens[2]);      // 心跳间隔
                if (tokens.size() >= 5)
                {
                    int64_t openTime = tokens[3].empty() ? 0 : std::stoll(tokens[3]);  // 系统开机时间戳
                    int64_t closeTime = tokens[4].empty() ? 0 : std::stoll(tokens[4]); // 系统关机时间戳
                }

                heartbeat(speed);
            }
            catch (const std::exception &e)
            {
                LOGE("Control", "配置参数解析错误:%s ", e.what());
            }
        }
        else
        {
            LOGE("Control", "配置参数格式无效 ");
        }
    }
}

/// @brief 刷新播放器
/// @param device_id
void Control::refresh(const std::string device_id)
{
    LOGI("Control", "刷新设备播放列表:%s ", device_id.c_str());
    auto playList = task_repository_.getPlayList(device_id);
    for (const auto &item : playList)
    {
        downloader_.add_task(*item);
    }
    if (display_->getDeviceId() == device_id)
    {
        display_->clear();
    }
}

/// @brief 文件下载完成回调
/// @param file_name
/// @param file_id
void Control::downloadCallback(const MediaItem &media, const std::string &local_path, bool success, const std::string &error)
{
    if (success)
    {
        if (display_->getDeviceId() == media.device_id)
        {
            display_->addMediaItem(media, local_path);
        }
    }
    else
    {
        LOGE("Control", "Failed:%s Error:%s", media.file_name, error);
    }
}

void Control::heartbeat(const std::int32_t &speed)
{
    // 如果线程已运行则，停止后重新启动
    if (daemon_thread_)
    {
        daemon_thread_->stop();
    }
    if (mqtt_client_)
    {
        daemon_thread_ = std::make_shared<DaemonThread>(mqtt_client_.get(), speed);
        daemon_thread_->start();
    }
    else
    {
        LOGE("Control", "MQTT客户端未初始化，无法启动心跳线程");
    }
}

void Control::reset_config()
{
    // 实现重置配置的逻辑
    if (mqtt_client_)
    {
        mqtt_client_->disconnect();
    }
}

void Control::show_config()
{
    display_->show_config();
}

void Control::update_url(const std::string &url)
{
    url_root_ = url;
    downloader_.update_url(url);
}
