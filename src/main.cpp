#include <cstdio>
#include <string>
#include <iostream>
#include "downloader.h"
#include "http_client.h"
#include "mqtt_client.h"
#include <control.h>
#include <GStreamerPlayer.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <cstdint>
#include <ImageDecoder.h>
#include <Framebuffer.h>

void display_image(const char *fb_device, const char *image_path)
{
    // 1. 打开framebuffer设备
    int fb_fd = open(fb_device, O_RDWR);
    if (fb_fd == -1)
    {
        perror("无法打开framebuffer设备");
        return;
    }

    // 2. 获取屏幕信息
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo))
    {
        perror("无法获取屏幕信息");
        close(fb_fd);
        return;
    }

    // 3. 计算framebuffer大小
    size_t fb_size = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8;

    // 4. 内存映射
    uint8_t *fb_ptr = (uint8_t *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_ptr == MAP_FAILED)
    {
        perror("内存映射失败");
        close(fb_fd);
        return;
    }

    // 5. 加载图片（这里需要实现图片解码）
    // 使用libpng、libjpeg等库
    ImageData img = ImageDecoder::decode(image_path);

    // 6. 将图片数据复制到framebuffer
    Framebuffer::draw_image_to_framebuffer(fb_ptr, vinfo, img);

    // 7. 清理
    munmap(fb_ptr, fb_size);
    close(fb_fd);
}

int main()
{
    std::string url_root = "http://114.55.52.37:8000/";
    std::string client_id = "6A10000000FF";

    // 创建工作目录
    std::filesystem::create_directory("data");

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

    // display_image("/dev/fb0", "/home/yuanjianting/player/data/files/default.jpg");

    // 模拟启动ui
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}