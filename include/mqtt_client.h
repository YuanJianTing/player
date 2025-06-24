#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>
#include <MQTTClient.h> // Paho MQTT C 头文件

class mqtt_client
{
public:
    using MessageCallback = std::function<void(const std::string &, const std::string &)>;

    mqtt_client(const std::string &mqtt_url,
                const std::string &client_id,
                const std::string &account,
                const std::string &password);
    ~mqtt_client();

    void setMessageCallback(MessageCallback callback);
    void publish(const std::string &command, const std::string &message, int qos = 1);
    bool isConnected() const;
    void disconnect();
    void connect();

private:
    void subscribe();
    void sendRegisterMessage();
    std::string urlEncode(const std::string &value);
    static void connectionLost(void *context, char *cause);
    static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message);
    static void deliveryComplete(void *context, MQTTClient_deliveryToken dt);

    std::string mqtt_url_;
    std::string client_id_;
    std::string account_;
    std::string password_;
    MessageCallback message_callback_;
    MQTTClient client_; // 明确使用全局命名空间的MQTTClient
    MQTTClient_connectOptions conn_opts_;
};

#endif // MQTT_CLIENT_H