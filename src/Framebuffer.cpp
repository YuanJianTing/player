#include "Framebuffer.h"
#include <cstdint>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <algorithm>
#include <iostream>

/**
 * 将像素绘制到 Framebuffer（支持alpha混合）
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

    // 提取alpha值
    uint8_t alpha = (pixel >> 24) & 0xFF;

    // 如果完全不透明，直接写入
    if (alpha == 0xFF)
    {
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
        return;
    }

    // 如果完全透明，不进行任何操作
    if (alpha == 0x00)
    {
        return;
    }

    // 需要alpha混合的情况
    switch (vinfo.bits_per_pixel)
    {
    case 16:
    { // RGB565
        // 读取目标像素
        uint16_t dest = *reinterpret_cast<uint16_t *>(pixel_ptr);
        // 解包目标像素
        uint8_t dest_r = (dest >> 11) & 0x1F;
        uint8_t dest_g = (dest >> 5) & 0x3F;
        uint8_t dest_b = dest & 0x1F;

        // 解包源像素
        uint8_t src_r = (pixel >> 19) & 0x1F; // 32->16位转换
        uint8_t src_g = (pixel >> 10) & 0x3F;
        uint8_t src_b = (pixel >> 3) & 0x1F;

        // alpha混合
        dest_r = (src_r * alpha + dest_r * (255 - alpha)) / 255;
        dest_g = (src_g * alpha + dest_g * (255 - alpha)) / 255;
        dest_b = (src_b * alpha + dest_b * (255 - alpha)) / 255;

        // 重新打包
        uint16_t rgb565 = (dest_r << 11) | (dest_g << 5) | dest_b;
        *reinterpret_cast<uint16_t *>(pixel_ptr) = rgb565;
        break;
    }
    case 24:
    { // BGR888 / RGB888
        // 读取目标像素
        uint8_t dest_r, dest_g, dest_b;
        if (vinfo.red.offset == 16)
        { // RGB888
            dest_r = pixel_ptr[0];
            dest_g = pixel_ptr[1];
            dest_b = pixel_ptr[2];
        }
        else
        { // BGR888
            dest_b = pixel_ptr[0];
            dest_g = pixel_ptr[1];
            dest_r = pixel_ptr[2];
        }

        // 解包源像素
        uint8_t src_r = (pixel >> 16) & 0xFF;
        uint8_t src_g = (pixel >> 8) & 0xFF;
        uint8_t src_b = pixel & 0xFF;

        // alpha混合
        dest_r = (src_r * alpha + dest_r * (255 - alpha)) / 255;
        dest_g = (src_g * alpha + dest_g * (255 - alpha)) / 255;
        dest_b = (src_b * alpha + dest_b * (255 - alpha)) / 255;

        // 重新写入
        if (vinfo.red.offset == 16)
        { // RGB888
            pixel_ptr[0] = dest_r;
            pixel_ptr[1] = dest_g;
            pixel_ptr[2] = dest_b;
        }
        else
        { // BGR888
            pixel_ptr[0] = dest_b;
            pixel_ptr[1] = dest_g;
            pixel_ptr[2] = dest_r;
        }
        break;
    }
    case 32:
    { // ARGB8888 / RGBA8888
        // 读取目标像素
        uint32_t dest = *reinterpret_cast<uint32_t *>(pixel_ptr);
        uint8_t dest_a, dest_r, dest_g, dest_b;

        if (vinfo.transp.offset == 24)
        { // RGBA8888
            dest_a = (dest >> 24) & 0xFF;
            dest_r = (dest >> 16) & 0xFF;
            dest_g = (dest >> 8) & 0xFF;
            dest_b = dest & 0xFF;
        }
        else
        { // ARGB8888
            dest_a = (dest >> 24) & 0xFF;
            dest_r = (dest >> 16) & 0xFF;
            dest_g = (dest >> 8) & 0xFF;
            dest_b = dest & 0xFF;
        }

        // 解包源像素
        uint8_t src_r = (pixel >> 16) & 0xFF;
        uint8_t src_g = (pixel >> 8) & 0xFF;
        uint8_t src_b = pixel & 0xFF;

        // 预乘alpha
        uint8_t combined_alpha = alpha + (dest_a * (255 - alpha) / 255);

        // alpha混合
        dest_r = (src_r * alpha + dest_r * dest_a * (255 - alpha) / 255) / combined_alpha;
        dest_g = (src_g * alpha + dest_g * dest_a * (255 - alpha) / 255) / combined_alpha;
        dest_b = (src_b * alpha + dest_b * dest_a * (255 - alpha) / 255) / combined_alpha;
        dest_a = combined_alpha;

        // 重新打包
        if (vinfo.transp.offset == 24)
        { // RGBA8888
            *reinterpret_cast<uint32_t *>(pixel_ptr) =
                (dest_a << 24) | (dest_r << 16) | (dest_g << 8) | dest_b;
        }
        else
        { // ARGB8888
            *reinterpret_cast<uint32_t *>(pixel_ptr) =
                (dest_a << 24) | (dest_r << 16) | (dest_g << 8) | dest_b;
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