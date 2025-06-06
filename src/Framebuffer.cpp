#include "Framebuffer.h"
#include <cstdint>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <algorithm>

/**
 * 将像素绘制到 Framebuffer
 * @param fb_ptr      Framebuffer 内存指针
 * @param vinfo       Framebuffer 屏幕信息
 * @param x           目标 X 坐标
 * @param y           目标 Y 坐标
 * @param pixel       32位 ARGB 格式的像素值
 */
void Framebuffer::draw_to_framebuffer(uint8_t *fb_ptr, const fb_var_screeninfo &vinfo,
                                      uint32_t x, uint32_t y, uint32_t pixel)
{
    // 检查坐标是否越界
    if (x >= vinfo.xres || y >= vinfo.yres)
    {
        return;
    }

    // 计算目标内存位置
    size_t offset = (y * vinfo.xres + x) * (vinfo.bits_per_pixel / 8);
    uint8_t *pixel_ptr = fb_ptr + offset;

    // 根据 Framebuffer 格式处理像素
    switch (vinfo.bits_per_pixel)
    {
    case 16:
    { // RGB565
        uint16_t r = (pixel >> 16) & 0xF8;
        uint16_t g = (pixel >> 8) & 0xFC;
        uint16_t b = (pixel) & 0xF8;
        uint16_t rgb565 = (r << 8) | (g << 3) | (b >> 3);
        *reinterpret_cast<uint16_t *>(pixel_ptr) = rgb565;
        break;
    }
    case 24:
    { // BGR888 / RGB888
        if (vinfo.red.offset == 16)
        {                                        // RGB888
            pixel_ptr[0] = (pixel >> 16) & 0xFF; // R
            pixel_ptr[1] = (pixel >> 8) & 0xFF;  // G
            pixel_ptr[2] = (pixel) & 0xFF;       // B
        }
        else
        {                                        // BGR888
            pixel_ptr[0] = (pixel) & 0xFF;       // B
            pixel_ptr[1] = (pixel >> 8) & 0xFF;  // G
            pixel_ptr[2] = (pixel >> 16) & 0xFF; // R
        }
        break;
    }
    case 32:
    { // ARGB8888 / RGBA8888
        if (vinfo.transp.offset == 24)
        { // RGBA8888
            *reinterpret_cast<uint32_t *>(pixel_ptr) =
                ((pixel >> 24) << 24) | // A
                ((pixel >> 16) << 16) | // R
                ((pixel >> 8) << 8) |   // G
                (pixel);                // B
        }
        else
        { // ARGB8888
            *reinterpret_cast<uint32_t *>(pixel_ptr) = pixel;
        }
        break;
    }
    default:
        // 不支持的格式，写入红色作为错误提示
        *reinterpret_cast<uint16_t *>(pixel_ptr) = 0xF800;
        break;
    }
}

/**
 * 优化版：批量绘制整个图片到 Framebuffer
 * @param fb_ptr      Framebuffer 内存指针
 * @param vinfo       Framebuffer 屏幕信息
 * @param img         要绘制的图片数据
 */
void Framebuffer::draw_image_to_framebuffer(uint8_t *fb_ptr, const fb_var_screeninfo &vinfo,
                                            const ImageData &img, const int offset_x, const int offset_y)
{
    // 计算每行字节数
    size_t fb_row_bytes = vinfo.xres * (vinfo.bits_per_pixel / 8);
    size_t img_row_bytes = img.width * img.channels;

    // 仅绘制可见部分
    // uint32_t draw_width = std::min(img.width, vinfo.xres);
    // uint32_t draw_height = std::min(img.height, vinfo.yres);
    uint32_t draw_width = std::min<uint32_t>(img.width, vinfo.xres);
    uint32_t draw_height = std::min<uint32_t>(img.height, vinfo.yres);

    for (uint32_t y = 0; y < draw_height; y++)
    {
        uint8_t *fb_row = fb_ptr + y * fb_row_bytes;
        const uint8_t *img_row = &img.pixels[y * img_row_bytes];

        for (uint32_t x = 0; x < draw_width; x++)
        {
            // 从图片中读取像素（假设图片是 RGBA）
            uint32_t r = img_row[x * img.channels];
            uint32_t g = img_row[x * img.channels + 1];
            uint32_t b = img_row[x * img.channels + 2];
            uint32_t a = (img.channels == 4) ? img_row[x * img.channels + 3] : 0xFF;

            // 合成 32位 ARGB 像素
            uint32_t pixel = (a << 24) | (r << 16) | (g << 8) | b;

            // 调用单个像素绘制
            draw_to_framebuffer(fb_ptr, vinfo, offset_x + x, offset_y + y, pixel);
        }
    }
}