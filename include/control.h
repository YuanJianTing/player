#ifndef CONTROL_H
#define CONTROL_H

#include <string>
#include "downloader.h"
#include "mqtt_client.h"
#include "daemon_thread.h"
#include "task_repository.h"
#include "GStreamerPlayer.h"
#include "display.h"

class Control
{
private:
    /* data */
    std::string url_root_;
    std::string client_id_;
    Downloader downloader_;
    TaskRepository task_repository_;
    std::shared_ptr<Display> display_;

    std::shared_ptr<MQTTClient> mqtt_client_;
    std::shared_ptr<DaemonThread> daemon_thread_;

    void handleMessage(const std::string &code, const std::string &body);
    void heartbeat(const std::int32_t &speed);
    void downloadCallback(const MediaItem &media, const std::string &local_path, bool success, const std::string &error);

    // void updateBackground(const std::string &file_id);

public:
    Control(const std::string &url_root, const std::string &client_id);
    ~Control();

    // 显示屏幕
    void show();
    // 启动控制器
    void start();
    // 刷新播放器
    void refresh(const std::string device_id);
    // 重置配置
    void reset_config();
    void show_config();
    void update_url(const std::string &url);
};
#endif // CONTROL_H