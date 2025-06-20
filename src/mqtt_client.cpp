// mqtt_client.cpp
#include "mqtt_client.h"
#include <iostream>
#include <json/json.h>
#include <sstream>
#include "Tools.h"
#include <cctype>
#include <iomanip>
#include <string>
#include <logger.h>

MQTTClient::MQTTClient(const std::string &mqtt_url,
                       const std::string &client_id,
                       const std::string &account,
                       const std::string &password)
    : mqtt_url_(mqtt_url),
      client_id_(client_id),
      account_(account),
      password_(password)
{

    LOGI("Display", "mqtt 地址：%s client_id：%s", mqtt_url_.c_str(), client_id_.c_str());

    // 创建MQTT客户端
    client_ = std::make_shared<mqtt::async_client>(mqtt_url_, client_id_);

    // 创建并设置回调
    callback_ = std::make_shared<MQTTCallback>(*this);
    client_->set_callback(*callback_);

    // 设置连接选项
    conn_opts_ = mqtt::connect_options();
    conn_opts_.set_keep_alive_interval(20);
    conn_opts_.set_clean_session(true);
    conn_opts_.set_user_name(account_);
    conn_opts_.set_password(password_);

    // conn_opts_.set_will

    connect();
}

void MQTTClient::handleMessage(mqtt::const_message_ptr msg)
{
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();

    // std::cout << "收到消息： topic=" << topic << " body:" << payload << std::endl;
    if (message_callback_)
    {
        if (payload.size() >= 4)
        {
            std::string code = payload.substr(0, 4);
            std::string body = payload.substr(4);
            message_callback_(code, body);
        }
    }
}

void MQTTClient::disconnect()
{
    if (client_ && client_->is_connected())
    {
        client_->disconnect()->wait();
    }
}

MQTTClient::~MQTTClient()
{
    if (client_ && client_->is_connected())
    {
        client_->disconnect()->wait();
    }
}

void MQTTClient::setMessageCallback(MessageCallback callback)
{
    message_callback_ = callback;
}

void MQTTClient::publish(const std::string &command,
                         const std::string &message,
                         int qos)
{
    if (!isConnected())
        return;

    std::string topic = "lcd/" + command;
    std::string payload = client_id_ + "&6A&" + urlEncode(message);
    // std::cout << "发送消息：" << payload << std::endl;
    try
    {
        auto msg = mqtt::make_message(topic, payload, qos, false);
        client_->publish(msg)->wait();
    }
    catch (const mqtt::exception &exc)
    {
        LOGE("Display", "Publish error：%s ", exc.what());
    }
}

bool MQTTClient::isConnected() const
{
    return client_ && client_->is_connected();
}

void MQTTClient::connect()
{
    try
    {
        client_->connect(conn_opts_)->wait();
        LOGI("Display", "服务器连接成功。");
        subscribe();
        sendRegisterMessage();
    }
    catch (const mqtt::exception &exc)
    {
        LOGE("Display", "Connection error：%s ", exc.what());
    }
}

void MQTTClient::subscribe()
{
    if (!isConnected())
        return;

    try
    {
        client_->subscribe(client_id_, 1)->wait();
    }
    catch (const mqtt::exception &exc)
    {
        LOGE("Display", "Subscribe error：%s ", exc.what());
    }
}

void MQTTClient::sendRegisterMessage()
{
    if (!isConnected())
        return;

    Json::Value root;
    root["MAC"] = client_id_;
    root["clientType"] = 2;
    root["version"] = Tools::get_version();
    root["shopCode"] = "0001";
    root["width"] = 800;
    root["height"] = 1280;
    root["IP"] = Tools::get_device_ip();
    root["deviceModel"] = "linux";
    root["firmware"] = Tools::get_firmware();
    root["SSID"] = Tools::get_current_wifi_ssid();
    root["hardware"] = "";

    Json::StreamWriterBuilder builder;
    const std::string payload = Json::writeString(builder, root);

    LOGI("Display", "注册设备：%s ", payload.c_str());

    try
    {

        publish("register", payload);
        // std::cout << "获取服务器配置 " << std::endl;
        // 获取服务器配置
        publish("get_config", "");
    }
    catch (const mqtt::exception &exc)
    {
        LOGE("Display", "Register message error：%s ", exc.what());
    }
}

std::string MQTTClient::urlEncode(const std::string &value)
{
    if (value.empty())
    {
        return value;
    }

    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto c : value)
    {
        // 保留字母数字和特定字符不编码
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        // 空格编码为 '+'
        else if (c == ' ')
        {
            escaped << '+';
        }
        // 其他字符进行百分号编码
        else
        {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}