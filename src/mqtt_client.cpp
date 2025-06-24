#include "mqtt_client.h"
#include <iostream>
#include <json/json.h>
#include <sstream>
#include "Tools.h"
#include <cctype>
#include <iomanip>
#include <string>
#include <logger.h>

mqtt_client::mqtt_client(const std::string &mqtt_url,
                         const std::string &client_id,
                         const std::string &account,
                         const std::string &password)
    : mqtt_url_(mqtt_url),
      client_id_(client_id),
      account_(account),
      password_(password)
{

    LOGI("Display", "mqtt 地址：%s client_id：%s", mqtt_url_.c_str(), client_id_.c_str());

    // 初始化MQTT客户端
    int rc;
    MQTTClient_create(&client_, mqtt_url_.c_str(), client_id_.c_str(),
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // 设置回调
    MQTTClient_setCallbacks(client_, this, connectionLost, messageArrived, deliveryComplete);

    // 设置连接选项
    conn_opts_.keepAliveInterval = 20;
    conn_opts_.cleansession = 1;
    conn_opts_.username = account_.c_str();
    conn_opts_.password = password_.c_str();
    conn_opts_.MQTTVersion = MQTTVERSION_3_1_1;

    connect();
}

mqtt_client::~mqtt_client()
{
    disconnect();
    MQTTClient_destroy(&client_);
}

void mqtt_client::connectionLost(void *context, char *cause)
{
    mqtt_client *self = static_cast<mqtt_client *>(context);
    LOGE("Display", "Connection lost: %s", cause ? cause : "unknown reason");
    // 尝试重新连接
    self->connect();
}

int mqtt_client::messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    mqtt_client *self = static_cast<mqtt_client *>(context);

    std::string payload(static_cast<char *>(message->payload), message->payloadlen);

    if (self->message_callback_)
    {
        if (payload.size() >= 4)
        {
            std::string code = payload.substr(0, 4);
            std::string body = payload.substr(4);
            self->message_callback_(code, body);
        }
    }

    MQTTClient_free(topicName);
    MQTTClient_freeMessage(&message);
    return 1;
}

void mqtt_client::deliveryComplete(void *context, MQTTClient_deliveryToken dt)
{
    // 消息发布完成回调
}

void mqtt_client::disconnect()
{
    if (isConnected())
    {
        MQTTClient_disconnect(client_, 10000);
    }
}

void mqtt_client::setMessageCallback(MessageCallback callback)
{
    message_callback_ = callback;
}

void mqtt_client::publish(const std::string &command,
                          const std::string &message,
                          int qos)
{
    if (!isConnected())
        return;

    std::string topic = "lcd/" + command;
    std::string payload = client_id_ + "&6A&" + urlEncode(message);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = const_cast<char *>(payload.c_str());
    pubmsg.payloadlen = static_cast<int>(payload.length());
    pubmsg.qos = qos;
    pubmsg.retained = 0;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(client_, topic.c_str(), &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        LOGE("Display", "Publish error: %d", rc);
    }
}

bool mqtt_client::isConnected() const
{
    return MQTTClient_isConnected(client_);
}

void mqtt_client::connect()
{
    int rc = MQTTClient_connect(client_, &conn_opts_);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        LOGE("Display", "Connection error: %d", rc);
        return;
    }

    LOGI("Display", "服务器连接成功。");
    subscribe();
    sendRegisterMessage();
}

void mqtt_client::subscribe()
{
    if (!isConnected())
        return;

    int rc = MQTTClient_subscribe(client_, client_id_.c_str(), 1);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        LOGE("Display", "Subscribe error: %d", rc);
    }
}

void mqtt_client::sendRegisterMessage()
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

    publish("register", payload);
    publish("get_config", "");
}

std::string mqtt_client::urlEncode(const std::string &value)
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
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else if (c == ' ')
        {
            escaped << '+';
        }
        else
        {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}