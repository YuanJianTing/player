#include "TextRenderer.h"
#include "stb_truetype.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>

TextRenderer::TextRenderer()
{
}

TextRenderer::~TextRenderer()
{
}

void TextRenderer::init(const TextRenderConfig &config)
{
    m_config = config;

    // 1. 读取字体文件
    std::ifstream file(config.font_path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open font file: " + config.font_path);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    m_ttf_buffer.resize(size);
    if (!file.read((char *)m_ttf_buffer.data(), size))
    {
        throw std::runtime_error("Failed to read font file: " + config.font_path);
    }

    // 2. 初始化 stb_truetype
    if (!stbtt_InitFont(&m_font, m_ttf_buffer.data(), 0))
    {
        throw std::runtime_error("Failed to initialize font: " + config.font_path);
    }

    // 3. 设置字体参数
    m_scale = stbtt_ScaleForPixelHeight(&m_font, config.size);
    stbtt_GetFontVMetrics(&m_font, &m_ascent, &m_descent, &m_lineGap);

    m_ascent = roundf(m_ascent * m_scale);
    m_descent = roundf(m_descent * m_scale);
    m_lineGap = roundf(m_lineGap * m_scale);
}

ImageData TextRenderer::render_text(const std::string &text)
{
    if (text.empty())
    {
        return ImageData{{}, 0, 0, 4};
    }

    // 1. 初始化尺寸计算
    int width = 0;
    int max_ascender = 0;
    int max_descender = 0;

    // 第一次遍历：计算文本总尺寸
    int pen_x = 0;
    for (char c : text)
    {
        int advance, leftBearing;
        stbtt_GetCodepointHMetrics(&m_font, c, &advance, &leftBearing);
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&m_font, c, m_scale, m_scale, &x0, &y0, &x1, &y1);

        width += roundf(advance * m_scale);
        max_ascender = std::max(max_ascender, -y0);
        max_descender = std::max(max_descender, y1);
    }

    // 计算最终尺寸
    const int height = max_ascender + max_descender;
    if (width <= 0 || height <= 0)
    {
        return ImageData{{}, 0, 0, 4};
    }

    // 2. 创建图像缓冲区
    ImageData result;
    result.width = width;
    result.height = height;
    result.channels = 4; // RGBA
    result.pixels.resize(width * height * 4, 0);

    // 第二次遍历：渲染每个字符
    pen_x = 0;
    for (char c : text)
    {
        int advance, leftBearing;
        stbtt_GetCodepointHMetrics(&m_font, c, &advance, &leftBearing);
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&m_font, c, m_scale, m_scale, &x0, &y0, &x1, &y1);

        int char_width, char_height, x_off, y_off;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&m_font, m_scale, m_scale, c, &char_width, &char_height, &x_off, &y_off);

        // 计算基线位置
        const int baseline = max_ascender;
        const int y_start = baseline + y_off; // 注意这里的 y_off 是负数
        const int x_start = pen_x;

        // 渲染到图像缓冲区
        for (int y = 0; y < char_height; ++y)
        {
            for (int x = 0; x < char_width; ++x)
            {
                const int img_x = x + x_start;
                const int img_y = y + y_start;

                if (img_x < 0 || img_x >= width || img_y < 0 || img_y >= height)
                    continue;

                const int pos = (img_y * width + img_x) * 4;
                const uint8_t alpha = bitmap[y * char_width + x];

                // 混合颜色（带抗锯齿）
                if (alpha > 0)
                {
                    const float alpha_f = alpha / 255.0f;
                    result.pixels[pos + 0] = static_cast<uint8_t>((m_config.color >> 16) & 0xFF) * alpha_f; // R
                    result.pixels[pos + 1] = static_cast<uint8_t>((m_config.color >> 8) & 0xFF) * alpha_f;  // G
                    result.pixels[pos + 2] = static_cast<uint8_t>((m_config.color >> 0) & 0xFF) * alpha_f;  // B
                    result.pixels[pos + 3] = alpha;                                                         // A
                }
            }
        }

        pen_x += roundf(advance * m_scale);
    }

    return result;
}
