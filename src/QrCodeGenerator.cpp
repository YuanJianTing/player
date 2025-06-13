#include "QrCodeGenerator.h"
#include <qrencode.h>
#include <png.h>
#include <ImageDecoder.h>
#include <vector>

#include <string>
#include <vector>
#include <cstring>
#include <iostream>

ImageData QrCodeGenerator::generate(
    const std::string &text,
    int size,
    int margin,
    uint32_t fg_color,
    uint32_t bg_color,
    ErrorCorrection level)
{
    // 1. 生成QR码
    QRcode *qr = QRcode_encodeString(
        text.c_str(),
        0, // 版本自动选择
        static_cast<QRecLevel>(level),
        QR_MODE_8, // 支持UTF-8
        1          // 大小写敏感
    );

    if (!qr)
    {
        throw std::runtime_error("Failed to generate QR code");
    }

    // 2. 计算图像尺寸
    const int qr_width = qr->width + 2 * margin;
    const int img_width = qr_width * size;
    const int img_height = qr_width * size;

    // 3. 准备图像数据 (RGBA格式)
    ImageData result;
    result.width = img_width;
    result.height = img_height;
    result.channels = 4; // 强制使用RGBA
    result.pixels.resize(img_width * img_height * 4);

    // 4. 填充背景色
    for (int i = 0; i < img_width * img_height; ++i)
    {
        result.pixels[i * 4 + 0] = (bg_color >> 24) & 0xFF; // R
        result.pixels[i * 4 + 1] = (bg_color >> 16) & 0xFF; // G
        result.pixels[i * 4 + 2] = (bg_color >> 8) & 0xFF;  // B
        result.pixels[i * 4 + 3] = bg_color & 0xFF;         // A
    }

    // 5. 绘制QR模块（带放大效果）
    for (int y = 0; y < qr->width; ++y)
    {
        for (int x = 0; x < qr->width; ++x)
        {
            if (qr->data[y * qr->width + x] & 1)
            {
                // 计算放大后的像素位置
                const int px = (x + margin) * size;
                const int py = (y + margin) * size;

                // 填充放大区域
                for (int dy = 0; dy < size; ++dy)
                {
                    for (int dx = 0; dx < size; ++dx)
                    {
                        const int pos = ((py + dy) * img_width + (px + dx)) * 4;
                        result.pixels[pos + 0] = (fg_color >> 24) & 0xFF; // R
                        result.pixels[pos + 1] = (fg_color >> 16) & 0xFF; // G
                        result.pixels[pos + 2] = (fg_color >> 8) & 0xFF;  // B
                        result.pixels[pos + 3] = fg_color & 0xFF;         // A
                    }
                }
            }
        }
    }

    QRcode_free(qr);
    return result;
}
