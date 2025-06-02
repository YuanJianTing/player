#ifndef CONTROL_H
#define CONTROL_H

#include <string>
#include "downloader.h"
#include "mqtt_client.h"
#include "daemon_thread.h"

class Control
{
private:
    /* data */
    std::string url_root_;
    std::string client_id_;
    Downloader downloader_;
    std::shared_ptr<MQTTClient> mqtt_client_;
    std::shared_ptr<DaemonThread> daemon_thread_;

    void handleMessage(const std::string& code, const std::string& body);
    void heartbeat(const std::int32_t& speed);

public:
    Control(const std::string& url_root,const std::string& client_id);
    ~Control();

    //启动控制器
    void start();
    //重置配置
    void reset_config();

};
#endif // CONTROL_H