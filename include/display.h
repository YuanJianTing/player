#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>
#include <gst/gst.h>
#include "GStreamerPlayer.h"
// #include "GStreamerImage.h"
#include <memory>
#include <vector>
#include "task_repository.h"
#include "GstPlayer.h"

class Display
{
private:
    /* data */
    std::string device_id_;
    const char *fb_device_;
    // GStreamerPlayer player_;
    GstPlayer player_;
    // GStreamerImage price_image_;
    // GStreamerImage background_image_;
    std::vector<MediaItem> media_items_;

    void updatePrice(const MediaItem &media, const std::string &local_path);
    void updateBackground(const MediaItem &media, const std::string &local_path);
    void display_image(const char *image_path);

public:
    Display(const std::string &client_id, const char *fb_device);
    ~Display();

    std::string getDeviceId();
    void addMediaItem(const MediaItem &media, const std::string &local_path);
    void clear();
    // void refresh(const std::vector<std::unique_ptr<MediaItem>> media_items); // 刷新显示
    void destroyed();
};
#endif // !DISPLAY_H
