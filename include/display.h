#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>
#include "GStreamerPlayer.h"
#include <memory>
#include <vector>
#include "task_repository.h"
#include "GstPlayer.h"
#include "ImageDecoder.h"
#include <mutex>
#include <linux/fb.h>
#include "TextRenderer.h"

class Display
{
private:
    struct FramebufferInfo
    {
        int fd = -1;
        size_t size = 0;
        uint8_t *mapped = nullptr;
        fb_var_screeninfo vinfo;
    };

    /* data */
    std::string device_id_;
    const char *fb_device_;
    GstPlayer player_;
    std::vector<MediaItem> media_items_;
    FramebufferInfo fb_info_;
    std::mutex fb_mutex_;

    std::unique_ptr<TextRenderer> m_text_renderer;

    void updatePrice(const MediaItem &media, const std::string &local_path);
    void updateBackground(const MediaItem &media, const std::string &local_path);
    void display_image(const std::string &image_path, const int offset_x, const int offset_y);
    void display_image_data(const ImageData &image_data, const int offset_x, const int offset_y);

    void init_framebuffer();
    void release_framebuffer();
    void ensure_framebuffer_mapped();

    // 文本渲染功能
    void draw_text(const std::string &text, int x, int y,
                   const TextRenderConfig &config = {});

    // 多行文本
    void draw_text_multi(const std::vector<std::string> &lines, int x, int y, int line_spacing = 5,
                         const TextRenderConfig &config = {});

    void clear_screen(uint32_t color = 0xFFFFFFFF);

public:
    Display(const std::string &client_id, const char *fb_device);
    ~Display();

    // 禁用拷贝和赋值
    Display(const Display &) = delete;
    Display &operator=(const Display &) = delete;

    std::string getDeviceId() const;
    void addMediaItem(const MediaItem &media, const std::string &local_path);
    void clear();
    // 显示配置
    void show_config();
    // 显示默认背景
    void show_info();
};
#endif // !DISPLAY_H
