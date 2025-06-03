#include "display.h"
#include <string>
#include "GStreamerPlayer.h"
#include "GStreamerImage.h"
#include <iostream>

Display::Display(const std::string &client_id, const Downloader &downloader) : device_id_(client_id), downloader_(downloader)
{

    player_.setErrorCallback([](GStreamerPlayer::ErrorType type, const std::string &msg)
                             { std::cerr << "Error (" << static_cast<int>(type) << "): " << msg << std::endl; });

    player_.setStateChangedCallback([](GStreamerPlayer::State state)
                                    {
        const char* states[] = { "NULL", "READY", "PAUSED", "PLAYING" };
        std::cout << "State changed to: " << states[static_cast<int>(state)] << std::endl; });

    player_.setEOSCallback([]()
                           {
                               std::cout << "End of stream reached" << std::endl;
                               // player->seek(0);
                           });
}

Display::~Display()
{
    destroyed();
}

void Display::refresh(const std::vector<std::unique_ptr<MediaItem>> media_items)
{
    media_items_ = media_items;
    // TODO: refresh display
    for (const auto &item : media_items)
    {
        if (item->type == 0 && item->group == 0)
        {
            // 更新背景图
            updateBackground(item);
        }
        else if (item->type == 0 && item->group == 1)
        {
            updatePrice(item);
        }
    }
}

void Display::updateBackground(const std::unique_ptr<MediaItem> &item)
{
    background_image_.updateImage(item->file_name);
}

void Display::updatePrice(const std::unique_ptr<MediaItem> &item)
{
}

void Display::destroyed()
{
    player_.stop();
}