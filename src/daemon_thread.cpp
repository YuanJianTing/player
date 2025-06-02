#include "daemon_thread.h"
#include "mqtt_client.h"
#include <iostream>
#include <algorithm>
#include <chrono>

DaemonThread::DaemonThread(MQTTClient* mqtt_client, int speed_seconds)
    : mqtt_client_(mqtt_client),
      speed_seconds_(speed_seconds),
      stop_flag_(false),
      thread_(nullptr) {}

DaemonThread::~DaemonThread() {
    stop();
}

void DaemonThread::start() {
    if (!thread_) {
        stop_flag_ = false;
        thread_ = std::make_unique<std::thread>(&DaemonThread::runLoop, this);
    }
}

void DaemonThread::stop() {
    stop_flag_ = true;
    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }
}

void DaemonThread::updateTask(const std::string& task_id) {
    task_id_ = task_id;
}

void DaemonThread::runLoop() {
    while (!stop_flag_) {
        run();
        int sleep_time = std::max(speed_seconds_, 60);
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
    }
}

void DaemonThread::run() {
    if (isConnected()) {
        std::string data = "wifibssid;" + task_id_;
        publish("heartbeat", data);
    } else {
        std::cout << "MQTT client is not connected" << std::endl;
    }
}

bool DaemonThread::isConnected() const {
    return mqtt_client_ != nullptr && mqtt_client_->isConnected();
}

void DaemonThread::publish(const std::string& topic, const std::string& data) {
    if (mqtt_client_) {
        mqtt_client_->publish(topic, data);
    }
}