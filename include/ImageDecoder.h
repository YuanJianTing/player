#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#include <string>
#include <vector>
#include <webp/decode.h>
#include <webp/demux.h>

struct ImageData
{
    std::vector<uint8_t> pixels; // 存储 RGB/RGBA 数据
    int width;                   // 图片宽度
    int height;                  // 图片高度
    int channels;                // 通道数 (3=RGB, 4=RGBA)
};

class ImageDecoder
{
public:
    // 解码图片（自动检测格式）
    static ImageData decode(const std::string &filepath);

    // 解码 PNG
    static ImageData decodePNG(const std::string &filepath);

    // 解码 JPEG
    static ImageData decodeJPEG(const std::string &filepath);

    // 解码 WEBP
    static bool decodeWebP(const std::string &filePath,
                           std::vector<uint8_t> &output,
                           int &width, int &height);
};

#endif // IMAGE_DECODER_H