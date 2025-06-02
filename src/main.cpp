#include <cstdio>
#include <string>
#include <iostream>
#include "downloader.h"
#include "http_client.h"
#include "mqtt_client.h"
#include <control.h>
#include <GStreamerPlayer.h>


int main()
{
    printf("%s 向你问好!\n", "eplayer");

    std::string url_root = "http://114.55.52.37:8000/";
    std::string client_id="6A10000000FF";

    //GStreamerPlayer player;


    Control control(url_root,client_id);
    //启动线程，执行后台任务
    // 创建并启动控制线程
    std::thread control_thread([&control]() {
        //启动线程，执行后台任务
        control.start();
    });

    // 分离线程（如果不需等待线程结束）
    control_thread.detach();

    //启动播放器
    std::cout << "启动播放器" << std::endl;

    std::string defaultVideoPath = "~/eplayer/庞各庄西瓜.mp4";

    // if (!player.load(defaultVideoPath)) {
    //     return 1;
    // }

    // std::cout << "Duration: " << player.getDuration() / 1000 << " ms" << std::endl;

    // if (!player.play()) {
    //     return 1;
    // }
    // 模拟启动ui
    std::this_thread::sleep_for(std::chrono::seconds(60));


    // std::string url  ="api/resource/5%e5%ba%9e%e5%90%84%e5%ba%84%e8%a5%bf%e7%93%9c.mp4?sign=WlW%2f%2f%2byoqqdUXEj88Fa3C%2fRG%2faNpWelc%2ferRNJmze26emQcGFuh3Vw%3d%3d";
    // std::string file_name = "庞各庄西瓜.mp4";
    // std::string file_id = "0";
    // std::string md5="ada4f5a096cd2bf06f832b6828ce7df9";
    
    // HttpClient client(url_root);

    // HttpClient::SystemInfo info;
    // std::string error;

    // if (client.getSystemInfo(info, error)) {
    //     std::cout << "系统信息获取成功:" << std::endl;
    //     std::cout << "应用名称: " << info.appName << std::endl;
    //     std::cout << "客户名称: " << info.customerName << std::endl;
    //     std::cout << "MQTT地址: " << info.mqtt << std::endl;
    // } else {
    //     std::cerr << "获取配置信息错误: " << error << std::endl;

    //     return 0;
    // }

    // MQTTClient mqtt_client("tcp://"+info.mqtt,client_id,"LCD","eTagTech@Pass");

    // mqtt_client.setMessageCallback([](const std::string& code, const std::string& body){
    //     std::cout << "Received message - Code: " << code << ", Body: " << body << std::endl;
    // });

    // 发布消息
    //client.publish("command", "test message");


    // Downloader downloader(url_root);
    
    // downloader.add_task(url,file_name,file_id,md5,
    //     [](const std::string& file_name, const std::string& file_id, bool success, const std::string& error) {
    //         if (success) {
    //              std::cout << "Downloaded: " << file_name << " ID: " << file_id << std::endl;
    //         }else{
    //             std::cout << "Failed: " << file_name << " ID: " << file_id << " Error: " << error << std::endl;
    //         }
    // });


    
    return 0;
}