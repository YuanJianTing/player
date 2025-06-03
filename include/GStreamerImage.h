#ifndef GSTREAMER_IMAGE_H
#define GSTREAMER_IMAGE_H

#include <string>
#include <gst/gst.h>

class GStreamerImage
{
private:
    /* data */
    GstElement *pipeline_;

public:
    GStreamerImage();
    ~GStreamerImage();

    void updateImage(const std::string image_path);
    void destroyed();
};

#endif // GSTREAMER_IMAGE_H
