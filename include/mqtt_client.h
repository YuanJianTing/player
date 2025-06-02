// mqtt_client.h
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include <mqtt/async_client.h>
#include <mqtt/connect_options.h>
#include <mqtt/message.h>
#include <cstdlib>
#include <curl/curl.h>

class MQTTClient {
public:
    using MessageCallback = std::function<void(const std::string& code, const std::string& body)>;

    MQTTClient(const std::string& mqtt_url, 
               const std::string& client_id,
               const std::string& account,
               const std::string& password);
    
    ~MQTTClient();

    void setMessageCallback(MessageCallback callback);
    void publish(const std::string& command, 
                 const std::string& message, 
                 int qos = 1);

    bool isConnected() const;
    void disconnect();

private:
    // 内部回调类
    class MQTTCallback : public mqtt::callback {
    public:
        MQTTCallback(MQTTClient& client) : client_(client) {}
        
        void message_arrived(mqtt::const_message_ptr msg) override {
            client_.handleMessage(msg);
        }
        
    private:
        MQTTClient& client_;
    };

    void connect();
    void subscribe();
    
    void sendRegisterMessage();
    void handleMessage(mqtt::const_message_ptr msg);
    static std::string urlEncode(const std::string& value);

    std::string mqtt_url_;
    std::string client_id_;
    std::string account_;
    std::string password_;
    MessageCallback message_callback_;
    std::shared_ptr<mqtt::async_client> client_;
    std::shared_ptr<MQTTCallback> callback_;
    mqtt::connect_options conn_opts_;
};

#endif // MQTT_CLIENT_H