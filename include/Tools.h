#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>
#include "ImageDecoder.h"

class Tools
{
public:
    // 获取存储目录
    static std::string get_work_dir();

    // 获取下载文件保存目录
    static std::string get_download_dir();

    // 获取任务保存目录
    static std::string get_repository_dir();

    static std::string get_device_ip();
    // 生成图片
    static ImageData create_image(const uint32_t color, const int img_width, const int img_height);
    // 获取wifi ssid
    static std::string get_current_wifi_ssid();
    // 获取当前软件版本号
    static std::string get_version();
    // 获取固件版本
    static std::string get_firmware();
    static std::string getExecutablePath();
};

#endif // TOOLS_H