#include "ImageDecoder.h"
#include <png.h>
#include <stdexcept>
#include <fstream>
#include <jpeglib.h>
#include <webp/decode.h>
#include <webp/demux.h>
#include <iostream>

ImageData ImageDecoder::decode(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open image file");
    }

    // 读取前 8 字节检测格式
    uint8_t header[8];
    file.read(reinterpret_cast<char *>(header), 8);
    file.close();

    // PNG 签名: \x89PNG\r\n\x1a\n
    if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G')
    {
        return decodePNG(filepath);
    }
    // JPEG 签名: \xFF\xD8\xFF
    else if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF)
    {
        return decodeJPEG(filepath);
    }
    else
    {
        std::vector<uint8_t> imageData;
        int width = 0, height = 0;

        if (!ImageDecoder::decodeWebP(filepath, imageData, width, height))
        {
            throw std::runtime_error("Unsupported image format");
        }
        return {imageData, width, height, 4}; // 假设 WebP 返回 RGBA 数据
    }
}

ImageData ImageDecoder::decodePNG(const std::string &filepath)
{
    FILE *fp = fopen(filepath.c_str(), "rb");
    if (!fp)
    {
        throw std::runtime_error("Failed to open PNG file");
    }

    // 检查 PNG 签名
    png_byte header[8];
    if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8))
    {
        fclose(fp);
        throw std::runtime_error("Not a PNG file");
    }

    // 初始化 PNG 结构
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png)
    {
        fclose(fp);
        throw std::runtime_error("Failed to create PNG read struct");
    }

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        throw std::runtime_error("Failed to create PNG info struct");
    }

    // 错误处理
    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("PNG decoding error");
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    // 获取图片信息
    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // 转换为 8-bit RGB/RGBA
    if (bit_depth == 16)
    {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
    {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    png_read_update_info(png, info);

    // 分配内存并读取数据
    std::vector<uint8_t> pixels(width * height * 4);
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = &pixels[y * width * 4];
    }

    png_read_image(png, row_pointers.data());
    png_read_end(png, nullptr);

    // 清理
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    return {pixels, width, height, 4}; // 返回 RGBA 数据
}

ImageData ImageDecoder::decodeJPEG(const std::string &filepath)
{
    FILE *fp = fopen(filepath.c_str(), "rb");
    if (!fp)
    {
        throw std::runtime_error("Failed to open JPEG file");
    }

    // 初始化 JPEG 解码器
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);

    // 读取 JPEG 头部
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        throw std::runtime_error("Invalid JPEG file");
    }

    // 开始解码
    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int channels = cinfo.output_components; // 3=RGB, 1=Grayscale

    // 分配内存
    std::vector<uint8_t> pixels(width * height * channels);
    uint8_t *row_ptr = pixels.data();

    // 逐行读取
    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, &row_ptr, 1);
        row_ptr += width * channels;
    }

    // 清理
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    return {pixels, width, height, channels}; // 返回 RGB 或 Grayscale 数据
}

bool ImageDecoder::decodeWebP(const std::string &filePath,
                              std::vector<uint8_t> &output,
                              int &width, int &height)
{
    // 读取文件数据
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        return false;

    std::vector<uint8_t> fileData((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());

    // 解码WebP
    WebPBitstreamFeatures features;
    if (WebPGetFeatures(fileData.data(), fileData.size(), &features) != VP8_STATUS_OK)
    {
        return false;
    }

    width = features.width;
    height = features.height;

    // 分配输出缓冲区
    output.resize(width * height * 4); // RGBA格式

    // 解码为RGBA
    uint8_t *decodedData = WebPDecodeRGBA(fileData.data(), fileData.size(), &width, &height);
    if (!decodedData)
        return false;

    // 复制数据到输出
    std::copy(decodedData, decodedData + output.size(), output.begin());
    WebPFree(decodedData);

    return true;
}