#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>
#include <gst/gst.h>
#include "GStreamerPlayer.h"
#include "GStreamerImage.h"
#include <memory>
#include <vector>
#include "task_repository.h"
#include "downloader.h"

class Display
{
private:
    /* data */
    std::string device_id_;
    Downloader downloader_; // 下载器
    GStreamerPlayer player_;
    GStreamerImage price_image_;
    GStreamerImage background_image_;
    std::vector<std::unique_ptr<MediaItem>> media_items_;

    void updatePrice(const std::unique_ptr<MediaItem> &item);
    void updateBackground(const std::unique_ptr<MediaItem> &item);

public:
    Display(const std::string &client_id, const Downloader &downloader);
    ~Display();

    void refresh(const std::vector<std::unique_ptr<MediaItem>> media_items); // 刷新显示
    void destroyed();
};
#endif // !DISPLAY_H
