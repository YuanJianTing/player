#include "display.h"
#include <string>
#include "GStreamerPlayer.h"
// #include "GStreamerImage.h"
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <cstdint>
#include <ImageDecoder.h>
#include <Framebuffer.h>

Display::Display(const std::string &client_id, const char *fb_device) : device_id_(client_id), fb_device_(fb_device)
{

    player_.setErrorCallback([](GStreamerPlayer::ErrorType type, const std::string &msg)
                             { std::cerr << "Error (" << static_cast<int>(type) << "): " << msg << std::endl; });

    player_.setStateChangedCallback([](GStreamerPlayer::State state)
                                    {
        const char* states[] = { "NULL", "READY", "PAUSED", "PLAYING" };
        std::cout << "State changed to: " << states[static_cast<int>(state)] << std::endl; });

    player_.setEOSCallback([]()
                           {
                               std::cout << "End of stream reached" << std::endl;
                               // player->seek(0);
                           });
}

Display::~Display()
{
    destroyed();
}

void Display::addMediaItem(const MediaItem &media, const std::string &local_path)
{
    media_items_.push_back(media);

    if (media.type == 0 && media.group == 0)
    {
        // 更新背景图
        updateBackground(media, local_path);
    }
    else if (media.type == 0 && media.group == 1)
    {
        updatePrice(media, local_path);
    }
    else
    {
        // std::string defaultVideoPath = "/home/yuanjianting/player/data/files/FA1747287368705.mp4";
        // if (!player_.load(defaultVideoPath))
        // {
        //     std::cout << "加载视频文件失败：" << defaultVideoPath << std::endl;
        //     return;
        // }

        // if (!player_.play())
        // {
        //     std::cout << "播放视频文件失败：" << defaultVideoPath << std::endl;
        // }
    }
}

void Display::updateBackground(const MediaItem &media, const std::string &local_path)
{
    // background_image_.updateImage(local_path);
    display_image(local_path.c_str());
}

void Display::updatePrice(const MediaItem &media, const std::string &local_path)
{
    // price_image_.updateImage(local_path);
    display_image(local_path.c_str());
}

void Display::display_image(const char *image_path)
{
    // 1. 打开framebuffer设备
    int fb_fd = open(fb_device_, O_RDWR);
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

void Display::clear()
{
    media_items_.clear();
}

std::string Display::getDeviceId()
{
    return device_id_;
}

void Display::destroyed()
{
    player_.stop();
}