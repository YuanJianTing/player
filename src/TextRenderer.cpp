#include "TextRenderer.h"
#include <stdexcept>
#include <ImageDecoder.h>
#include <vector>
#include <memory>

TextRenderer::TextRenderer()
{
    if (FT_Init_FreeType(&m_ft))
        throw std::runtime_error("Could not init FreeType");
}

TextRenderer::~TextRenderer()
{
    if (m_face)
        FT_Done_Face(m_face);
    FT_Done_FreeType(m_ft);
}

void TextRenderer::init(const TextRenderConfig &config)
{
    m_config = config;

    if (FT_New_Face(m_ft, config.font_path.c_str(), 0, &m_face))
        throw std::runtime_error("Failed to load font: " + config.font_path);

    FT_Set_Pixel_Sizes(m_face, 0, config.size);
}

ImageData TextRenderer::render_text(const std::string &text)
{
    if (text.empty())
    {
        return ImageData{{}, 0, 0, 4};
    }

    // 初始化尺寸计算
    int width = 0;
    int max_ascender = 0;
    int max_descender = 0;
    int max_height = 0;

    // 第一次遍历：计算文本总尺寸
    for (char c : text)
    {
        if (FT_Load_Char(m_face, c, FT_LOAD_DEFAULT))
        {
            continue; // 跳过无法加载的字符
        }

        width += (m_face->glyph->advance.x >> 6);
        max_ascender = std::max(max_ascender, m_face->glyph->bitmap_top);
        max_descender = std::max(max_descender,
                                 static_cast<int>(m_face->glyph->metrics.height >> 6) - m_face->glyph->bitmap_top);
        max_height = std::max(max_height, static_cast<int>(m_face->glyph->metrics.height >> 6));
    }

    // 计算最终尺寸（包含可能的行间距）
    const int height = max_ascender + max_descender;
    if (width <= 0 || height <= 0)
    {
        return ImageData{{}, 0, 0, 4};
    }

    // 创建图像缓冲区
    ImageData result;
    result.width = width;
    result.height = height;
    result.channels = 4; // RGBA
    result.pixels.resize(width * height * 4, 0);

    // 第二次遍历：渲染每个字符
    int pen_x = 0;
    for (char c : text)
    {
        if (FT_Load_Char(m_face, c, FT_LOAD_RENDER))
        {
            continue;
        }

        FT_GlyphSlot glyph = m_face->glyph;
        FT_Bitmap *bitmap = &glyph->bitmap;

        // 计算基线位置
        const int baseline = max_ascender;
        const int y_start = baseline - glyph->bitmap_top;
        const int y_end = y_start + bitmap->rows;

        // 渲染到图像缓冲区
        for (int y = 0; y < bitmap->rows; ++y)
        {
            for (int x = 0; x < bitmap->width; ++x)
            {
                const int img_y = y + y_start;
                if (img_y < 0 || img_y >= height)
                    continue;

                const int img_x = x + pen_x + glyph->bitmap_left;
                if (img_x < 0 || img_x >= width)
                    continue;

                const int pos = (img_y * width + img_x) * 4;
                const uint8_t alpha = bitmap->buffer[y * bitmap->pitch + x];

                // 混合颜色（带抗锯齿）
                if (alpha > 0)
                {
                    const float alpha_f = alpha / 255.0f;
                    result.pixels[pos + 0] = static_cast<uint8_t>((m_config.color >> 24) & 0xFF) * alpha_f; // R
                    result.pixels[pos + 1] = static_cast<uint8_t>((m_config.color >> 16) & 0xFF) * alpha_f; // G
                    result.pixels[pos + 2] = static_cast<uint8_t>((m_config.color >> 8) & 0xFF) * alpha_f;  // B
                    result.pixels[pos + 3] = alpha;                                                         // A
                }
            }
        }

        pen_x += glyph->advance.x >> 6;
    }

    return result;
}

ImageData TextRenderer::render_bg_text(const std::string &text)
{
    if (text.empty())
    {
        return ImageData{{}, 0, 0, 4};
    }

    // 初始化尺寸计算
    int width = 0;
    int max_ascender = 0;
    int max_descender = 0;

    // 第一次遍历：计算文本总尺寸
    for (char c : text)
    {
        if (FT_Load_Char(m_face, c, FT_LOAD_DEFAULT))
        {
            continue;
        }

        width += (m_face->glyph->advance.x >> 6);
        max_ascender = std::max(max_ascender, m_face->glyph->bitmap_top);
        max_descender = std::max(max_descender,
                                 static_cast<int>(m_face->glyph->metrics.height >> 6) - m_face->glyph->bitmap_top);
    }

    const int height = max_ascender + max_descender;
    if (width <= 0 || height <= 0)
    {
        return ImageData{{}, 0, 0, 4};
    }

    // 创建图像缓冲区并填充背景色
    ImageData result;
    result.width = width;
    result.height = height;
    result.channels = 4;
    result.pixels.resize(width * height * 4);

    // 填充背景色
    uint8_t bg_r = (m_config.bg_color >> 24) & 0xFF;
    uint8_t bg_g = (m_config.bg_color >> 16) & 0xFF;
    uint8_t bg_b = (m_config.bg_color >> 8) & 0xFF;
    uint8_t bg_a = m_config.bg_color & 0xFF;

    for (int i = 0; i < width * height; ++i)
    {
        result.pixels[i * 4] = bg_r;
        result.pixels[i * 4 + 1] = bg_g;
        result.pixels[i * 4 + 2] = bg_b;
        result.pixels[i * 4 + 3] = bg_a;
    }

    // 第二次遍历：渲染每个字符
    int pen_x = 0;
    uint8_t fg_r = (m_config.color >> 24) & 0xFF;
    uint8_t fg_g = (m_config.color >> 16) & 0xFF;
    uint8_t fg_b = (m_config.color >> 8) & 0xFF;

    for (char c : text)
    {
        if (FT_Load_Char(m_face, c, FT_LOAD_RENDER))
        {
            continue;
        }

        FT_GlyphSlot glyph = m_face->glyph;
        FT_Bitmap *bitmap = &glyph->bitmap;
        const int baseline = max_ascender;
        const int y_start = baseline - glyph->bitmap_top;

        // 渲染字符
        for (int y = 0; y < bitmap->rows; ++y)
        {
            for (int x = 0; x < bitmap->width; ++x)
            {
                const int img_y = y + y_start;
                if (img_y < 0 || img_y >= height)
                    continue;

                const int img_x = x + pen_x + glyph->bitmap_left;
                if (img_x < 0 || img_x >= width)
                    continue;

                const int pos = (img_y * width + img_x) * 4;
                const uint8_t alpha = bitmap->buffer[y * bitmap->pitch + x];

                if (alpha > 0)
                {
                    // Alpha混合公式: result = foreground * alpha + background * (1 - alpha)
                    const float alpha_normalized = alpha / 255.0f;
                    const float inv_alpha = 1.0f - alpha_normalized;

                    result.pixels[pos] = static_cast<uint8_t>(fg_r * alpha_normalized + bg_r * inv_alpha);
                    result.pixels[pos + 1] = static_cast<uint8_t>(fg_g * alpha_normalized + bg_g * inv_alpha);
                    result.pixels[pos + 2] = static_cast<uint8_t>(fg_b * alpha_normalized + bg_b * inv_alpha);
                    result.pixels[pos + 3] = static_cast<uint8_t>(255 * alpha_normalized + bg_a * inv_alpha);
                }
            }
        }

        pen_x += glyph->advance.x >> 6;
    }

    return result;
}
