#ifndef QRCODE_GENERAATOR_H
#define QRCODE_GENERAATOR_H

#include <string>
#include <vector>
#include <stdexcept>
#include <ImageDecoder.h>

class QrCodeGenerator
{
public:
    enum class ErrorCorrection
    {
        Low = 0,      // QR_ECLEVEL_L
        Medium = 1,   // QR_ECLEVEL_M
        Quartile = 2, // QR_ECLEVEL_Q
        High = 3      // QR_ECLEVEL_H
    };

    /**
     * 生成二维码图像数据
     * @param text 要编码的文本
     * @param size 每个模块的像素数（放大倍数）
     * @param margin 二维码边距（模块数）
     * @param fg_color 前景色（RGB/RGBA）
     * @param bg_color 背景色（RGB/RGBA）
     * @param level 纠错等级
     */
    static ImageData generate(
        const std::string &text,
        int size = 5,
        int margin = 4,
        uint32_t fg_color = 0x000000FF, // RGBA: 黑色
        uint32_t bg_color = 0xFFFFFFFF, // RGBA: 白色
        ErrorCorrection level = ErrorCorrection::Medium);
};
#endif // QRCODE_GENERAATOR_H