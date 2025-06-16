#include "display.h"
#include <string>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <cstdint>
#include <ImageDecoder.h>
#include <Framebuffer.h>
#include <Tools.h>
#include <QrCodeGenerator.h>

Display::Display(const std::string &client_id, const char *fb_device) : device_id_(client_id), fb_device_(fb_device), m_text_renderer(std::make_unique<TextRenderer>())
{
    
    init_framebuffer();
    // 设置回调
    // player_.set_eos_callback([]()
    //                          { std::cout << "\nEnd of stream" << std::endl; });
}

Display::~Display()
{
    release_framebuffer();
    // player_.stop();
}

void Display::init_framebuffer()
{
    std::lock_guard<std::mutex> lock(fb_mutex_);

    // 打开framebuffer设备
    fb_info_.fd = open(fb_device_, O_RDWR);
    if (fb_info_.fd == -1)
    {
        throw std::runtime_error("无法打开framebuffer设备: " + std::string(strerror(errno)));
    }

    // 获取屏幕信息
    if (ioctl(fb_info_.fd, FBIOGET_VSCREENINFO, &fb_info_.vinfo))
    {
        close(fb_info_.fd);
        throw std::runtime_error("无法获取屏幕信息: " + std::string(strerror(errno)));
    }

    // 计算framebuffer大小
    fb_info_.size = fb_info_.vinfo.yres_virtual * fb_info_.vinfo.xres_virtual *
                    fb_info_.vinfo.bits_per_pixel / 8;
}

void Display::release_framebuffer()
{
    std::lock_guard<std::mutex> lock(fb_mutex_);

    if (fb_info_.mapped)
    {
        munmap(fb_info_.mapped, fb_info_.size);
        fb_info_.mapped = nullptr;
    }

    if (fb_info_.fd != -1)
    {
        close(fb_info_.fd);
        fb_info_.fd = -1;
    }
}

void Display::ensure_framebuffer_mapped()
{
    std::lock_guard<std::mutex> lock(fb_mutex_);

    if (!fb_info_.mapped)
    {
        fb_info_.mapped = static_cast<uint8_t *>(
            mmap(0, fb_info_.size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_info_.fd, 0));

        if (fb_info_.mapped == MAP_FAILED)
        {
            fb_info_.mapped = nullptr;
            throw std::runtime_error("内存映射失败: " + std::string(strerror(errno)));
        }
    }
}

void Display::show_info()
{
    std::string ip = Tools::get_device_ip();
    std::cout << "设备ip " << ip << "屏幕id:" << fb_info_.fd << std::endl;
    std::cout << "设备分辨率： " << fb_info_.vinfo.xres << "x" << fb_info_.vinfo.yres << std::endl;
    try
    {
        clear_screen();

        std::string data = device_id_;
        if (!ip.empty())
        {
            data += ";" + ip;
        }

        // 生成RGBA格式的二维码
        ImageData qr_img = QrCodeGenerator::generate(
            data,
            8,
            4,
            0x000000FF, // 黑色前景
            0xFFFFFF00  // 白色背景（透明）
        );
        // 计算居中位置
        int x = (fb_info_.vinfo.xres - qr_img.width) / 2;
        int y = (fb_info_.vinfo.yres - qr_img.height) / 2;

        display_image_data(qr_img, x, y);

        std::vector<std::string> info = {
            device_id_,
            ip};
        draw_text_multi(info, x + 10, y + qr_img.height + 10, 8);

        draw_text(Tools::get_version(), x + 60, fb_info_.vinfo.yres - 50);
    }
    catch (const std::exception &e)
    {
        std::cerr << "初始化显示配置错误: " << e.what() << std::endl;
    }
}

void Display::show_config()
{
    std::string local_path = "static/etag_bg.png";
    display_image(local_path.c_str(), 0, 0);

    // 显示wifi区域
    try
    {
        std::string ip = Tools::get_device_ip();

        int window_width = 800;                  // fb_info_.vinfo.xres
        int window_height = fb_info_.vinfo.yres; // 1280
        ImageData result = Tools::create_image(0xFFFFFFFF, window_width - 140, window_height - 240);
        display_image_data(result, 40, 100);

        // config wifi
        draw_text("Configure  WIFI", 140, 120, {.size = 40});

        if (ip.empty())
        {
            draw_text("SSID:     ESLAP", 50, 190, {.size = 32});
            draw_text("PASSWORD: etag2018", 50, 250, {.size = 32});

            std::vector<std::string> info = {
                "The device will automatically scan and",
                " connect to WiFi"};

            draw_text_multi(info, 50, 300, 8, {.color = 0x1989FAFF, .size = 28});
        }
        else
        {
            std::string ssid = Tools::get_current_wifi_ssid();
            std::cout << "设备ip " << ip << " ssid: " << ssid << std::endl;
            // draw_text(ssid, 50, 190, {.size = 32});
            draw_text("SSID:     " + ssid, 50, 190, {.size = 32});
        }

        // 显示设备二维码
        std::string data = device_id_;
        if (!ip.empty())
        {
            data += ";" + ip;
        }

        // 生成RGBA格式的二维码
        ImageData qr_img = QrCodeGenerator::generate(
            data,
            8,
            4,
            0x000000FF, // 黑色前景
            0xFFFFFF00  // 白色背景（透明）
        );
        // 计算居中位置
        int x = (window_width - qr_img.width) / 2;
        display_image_data(qr_img, x - 40, 360);
        int y = 380 + qr_img.height;
        draw_text(device_id_, 210, y, {.size = 40});
        if (!ip.empty())
        {
            draw_text(ip, 210, y + 48, {.size = 40});
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "show_config错误: " << e.what() << std::endl;
    }
}

void Display::addMediaItem(const MediaItem &media, const std::string &local_path)
{
    media_items_.push_back(media);
    if (media.type == 0)
    {
        if (media.group == 0)
        {
            // std::cout << "背景图：" << local_path << std::endl;
            updateBackground(media, local_path);
        }
        else if (media.group == 1 || media.group == 99)
        {
            // std::cout << "价格图：" << local_path << std::endl;
            updatePrice(media, local_path);
        }
    }
    else
    {

        std::cout << "加载视频文件：" << local_path << std::endl;
        // player_.add_uri("file://" + local_path);
        // if (player_.getState() != GstPlayer::State::PLAYING)
        // {
        //     player_.set_video_display({media.left, media.top, media.width, media.height});
        //     std::cout << "没有开始播放，启动播放" << std::endl;
        //     //  开始播放
        //     player_.play();
        // }
    }
}

void Display::updateBackground(const MediaItem &media, const std::string &local_path)
{
    if (background_path_ == local_path)
        return;
    background_path_ = local_path;
    display_image(local_path.c_str(), media.left, media.top);

    // 绘制价格图片
    for (const auto &item : media_items_)
    {
        if (item.type == 0 && (item.group == 1 || item.group == 99))
        {
            if (price_path_.empty())
            {
                break;
            }
            updatePrice(item, price_path_);
            break;
        }
    }
}

void Display::updatePrice(const MediaItem &media, const std::string &local_path)
{
    price_path_ = local_path;
    display_image(local_path.c_str(), media.left, media.top);
}

void Display::display_image(const std::string &image_path, const int offset_x, const int offset_y)
{
    try
    {
        ImageData img = ImageDecoder::decode(image_path.c_str());
        // std::cout << "image channels " << img.channels << std::endl;
        display_image_data(img, offset_x, offset_y);
    }
    catch (const std::exception &e)
    {
        std::cerr << "图片显示错误: " << e.what() << std::endl;
    }
}

void Display::display_image_data(const ImageData &image_data, const int offset_x, const int offset_y)
{
    // 参数验证
    if (offset_x < 0 || offset_y < 0)
    {
        throw std::invalid_argument("显示位置不能为负数");
    }

    if (image_data.width <= 0 || image_data.height <= 0 || image_data.pixels.empty())
    {
        throw std::invalid_argument("无效的图像数据");
    }

    ensure_framebuffer_mapped();

    try
    {
        Framebuffer::draw_image_to_framebuffer(
            fb_info_.mapped,
            fb_info_.vinfo,
            image_data,
            offset_x,
            offset_y);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("绘制到framebuffer失败: " + std::string(e.what()));
    }
}

void Display::clear_screen(uint32_t color)
{
    try
    {
        ImageData result = Tools::create_image(color, fb_info_.vinfo.xres, fb_info_.vinfo.yres);
        display_image_data(result, 0, 0);
    }
    catch (const std::exception &e)
    {
        std::cerr << "clear_screen错误: " << e.what() << std::endl;
    }
}

void Display::draw_text(const std::string &text, int x, int y,
                        const TextRenderConfig &config)
{
    try
    {
        m_text_renderer->init(config);
        ImageData text_img = m_text_renderer->render_text(text);
        display_image_data(text_img, x, y);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Text render error: " << e.what() << std::endl;
    }
}

void Display::draw_text_multi(const std::vector<std::string> &lines, int x, int y, int line_spacing,
                              const TextRenderConfig &config)
{
    m_text_renderer->init(config);

    // 获取行高
    ImageData sample = m_text_renderer->render_text("A");
    int line_height = sample.height + line_spacing; // 加5像素行间距

    for (const auto &line : lines)
    {
        draw_text(line, x, y, config);
        y += line_height;
    }
}

void Display::clear()
{
    // player_.clear_list();
    media_items_.clear();

    // if (player_.getState() == GstPlayer::State::PLAYING)
    //     player_.stop();
}

std::string Display::getDeviceId() const
{
    return device_id_;
}
