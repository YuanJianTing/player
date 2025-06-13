#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ft2build.h>
#include <ImageDecoder.h>

#include FT_FREETYPE_H

struct TextRenderConfig
{
    std::string font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    uint32_t color = 0x000000FF;         // 字体颜色 (RGBA)
    uint32_t bg_color = 0x00000000;      // 背景颜色 (RGBA) 0xFFFFFFFF 白色背景  透明 0x00000000
    int size = 24;                       // 字体大小(像素)
    int outline = 0;                     // 描边宽度
    uint32_t outline_color = 0x000000FF; // 描边颜色
};

class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();

    void init(const TextRenderConfig &config);
    ImageData render_text(const std::string &text);
    ImageData render_bg_text(const std::string &text);

private:
    FT_Library m_ft;
    FT_Face m_face;
    TextRenderConfig m_config;
};