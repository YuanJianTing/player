#pragma once
#include <string>
#include <vector>
#include <memory>
// #include <ft2build.h>
#include <ImageDecoder.h>

// #include FT_FREETYPE_H

struct TextRenderConfig
{
    std::string font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    uint32_t color = 0x000000FF;    // 字体颜色 (RGBA)
    uint32_t bg_color = 0x00000000; // 背景颜色 (RGBA) 0xFFFFFFFF 白色背景  透明 0x00000000
    int size = 24;                  // 字体大小(像素)
};

class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();

    void init(const TextRenderConfig &config);
    ImageData render_text(const std::string &text);

private:
    TextRenderConfig m_config;
    std::vector<unsigned char> m_ttf_buffer;
    stbtt_fontinfo m_font;
    float m_scale;
    int m_ascent;
    int m_descent;
    int m_lineGap;
};