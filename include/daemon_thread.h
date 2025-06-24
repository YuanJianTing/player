#ifndef DAEMON_THREAD_H
#define DAEMON_THREAD_H

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <mqtt_client.h>

class DaemonThread
{
public:
    /**
     * @brief 构造函数
     * @param mqtt_client MQTT客户端指针
     * @param speed_seconds 心跳间隔时间(秒)
     */
    DaemonThread(mqtt_client *mqtt_client, int speed_seconds);

    /**
     * @brief 析构函数，自动停止线程
     */
    ~DaemonThread();

    // 删除拷贝构造和赋值操作
    DaemonThread(const DaemonThread &) = delete;
    DaemonThread &operator=(const DaemonThread &) = delete;

    /**
     * @brief 启动心跳线程
     */
    void start();

    /**
     * @brief 停止心跳线程
     */
    void stop();

    /**
     * @brief 更新任务ID
     * @param task_id 新的任务ID
     */
    void updateTask(const std::string &task_id);

protected:
    /**
     * @brief 线程运行循环
     */
    void runLoop();

    /**
     * @brief 执行单次心跳
     */
    void run();

private:
    /**
     * @brief 检查MQTT连接状态
     * @return 是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 发布MQTT消息
     * @param topic 主题
     * @param data 消息内容
     */
    void publish(const std::string &topic, const std::string &data);

    mqtt_client *mqtt_client_;            // MQTT客户端指针
    int speed_seconds_;                   // 心跳间隔秒数
    std::atomic<bool> stop_flag_;         // 线程停止标志
    std::unique_ptr<std::thread> thread_; // 使用unique_ptr管理线程
    std::string task_id_;                 // 当前任务ID
};

#endif // DAEMON_THREAD_H