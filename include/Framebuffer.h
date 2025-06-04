#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <string>
#include <vector>
#include <cstdint>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "ImageDecoder.h"

class Framebuffer
{
public:
    /**
     * 将像素绘制到 Framebuffer
     * @param fb_ptr      Framebuffer 内存指针
     * @param vinfo       Framebuffer 屏幕信息
     * @param x           目标 X 坐标
     * @param y           目标 Y 坐标
     * @param pixel       32位 ARGB 格式的像素值
     */
    static void draw_to_framebuffer(uint8_t *fb_ptr, const fb_var_screeninfo &vinfo,
                                    uint32_t x, uint32_t y, uint32_t pixel);

    /**
     * 优化版：批量绘制整个图片到 Framebuffer
     * @param fb_ptr      Framebuffer 内存指针
     * @param vinfo       Framebuffer 屏幕信息
     * @param img         要绘制的图片数据
     */
    static void draw_image_to_framebuffer(uint8_t *fb_ptr, const fb_var_screeninfo &vinfo,
                                          const ImageData &img);
};

#endif // FRAME_BUFFER_H