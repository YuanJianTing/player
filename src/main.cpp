#include <cstdio>
#include <string>
#include <iostream>
#include <control.h>
#include "Tools.h"
#include <GstConcatPlayer.h>

int main()
{

    // putenv("GST_DEBUG=3"); // 设置中等详细级别的调试输出
    // GstConcatPlayer player;
    // player.set_window_size(0, 0, 800, 1280);
    // player.add_uri("file:///data/download/26854ae43ae8c37d1d3628c0ba1457ca.mp4");
    // player.add_uri("file:///data/download/4ee6c886a6c0738d1b072d16d0444906.mp4");
    // player.play();

    // GMainLoop *main_loop_ = g_main_loop_new(nullptr, FALSE);
    // g_main_loop_run(main_loop_);

    // return 0;

    // std::string url_root = "http://192.168.4.26:4011/";
    std::string url_root = "http://114.55.52.37:8000/";
    std::string client_id = "6A10000000FF";

#if defined(debug) || defined(DEBUG)
    // 设置 日志输出
    putenv("GST_DEBUG=3"); // 设置中等详细级别的调试输出
// std::cout << "Release version" << std::endl;
#endif

    // 创建工作目录 EPLAYER_DIR  export EPLAYER_DIR=/mnt/user_downloads
    std::string work_dir = Tools::get_work_dir();
    std::filesystem::create_directory(work_dir);

    Control control(url_root, client_id);
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

    control.show();

    // 保持运行
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        // 检查状态
        //  control.check();
    }
    return 0;
}