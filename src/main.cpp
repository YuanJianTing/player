#include <cstdio>
#include <string>
#include <iostream>
#include "downloader.h"
#include "http_client.h"
#include "mqtt_client.h"
#include <control.h>
#include <GStreamerPlayer.h>

#include <gst/gst.h>

int main()
{
    std::string url_root = "http://114.55.52.37:8000/";
    std::string client_id = "6A10000000FF";

    // 创建工作目录
    std::filesystem::create_directory("data");

    GStreamerPlayer player;

    Control control(url_root, client_id, player);
    // 启动线程，执行后台任务
    // 创建并启动控制线程
    std::thread control_thread([&control]()
                               {
        //启动线程，执行后台任务
        control.start(); });

    // 分离线程（如果不需等待线程结束）
    control_thread.detach();

    // 启动播放器
    std::cout << "启动播放器" << std::endl;

    std::string defaultVideoPath = "data/files/1.mp4";

    // control.refresh(client_id);

    if (!player.load(defaultVideoPath))
    {
        std::cout << "load failed" << std::endl;
        return 1;
    }

    std::cout << "Duration: " << player.getDuration() / 1000 << " ms" << std::endl;

    if (!player.play())
    {
        std::cout << "play failed" << std::endl;
        return 1;
    }
    // 模拟启动ui
    // std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}