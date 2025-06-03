#include "display.h"
#include <string>
#include "GStreamerPlayer.h"
#include "GStreamerImage.h"
#include <iostream>

Display::Display(const std::string &client_id) : device_id_(client_id)
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

void Display::addMediaItem(const MediaItem &media, const std::string &local_path)
{
    media_items_.push_back(media);

    if (media.type == 0 && media.group == 0)
    {
        // 更新背景图
        updateBackground(media, local_path);
    }
    else if (media.type == 0 && media.group == 1)
    {
        updatePrice(media, local_path);
    }
    else
    {
        player_.load()
    }
}

void Display::updateBackground(const MediaItem &media, const std::string &local_path)
{
    background_image_.updateImage(local_path);
}

void Display::updatePrice(const MediaItem &media, const std::string &local_path)
{
    price_image_.updateImage(local_path);
}

void Display::clear()
{
    media_items_.clear();
}

std::string Display::getDeviceId()
{
    return device_id_;
}

void Display::destroyed()
{
    player_.stop();
}