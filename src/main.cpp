#include <cstdio>
#include <string>
#include <iostream>
#include <control.h>
#include "Tools.h"
#include "logger.h"

int main()
{
    std::string url_root = "http://192.168.4.26:4011/";
    // std::string url_root = "http://114.55.52.37:8000/";
    // std::string url_root = "";
    std::string client_id = "6A10000000FF";

    // 创建工作目录 EPLAYER_DIR  export EPLAYER_DIR=/mnt/user_downloads
    std::string work_dir = Tools::get_work_dir();

    std::filesystem::create_directory(work_dir);

    Control control(url_root, client_id);

    if (url_root.empty())
    {
        control.show_config();
    }

    // 启动线程，执行后台任务
    // 创建并启动控制线程
    std::thread control_thread([&control]()
                               {
        //启动线程，执行后台任务
        control.start(); });

    // 分离线程（如果不需等待线程结束）
    control_thread.detach();
    //  启动播放器
    LOGI("Startup", "启动播放器,工作目录：%s", work_dir.c_str());

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