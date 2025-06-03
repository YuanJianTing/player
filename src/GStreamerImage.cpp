#include "GStreamerImage.h"
#include <string>
#include <gst/gst.h>
#include <iostream>

GStreamerImage::GStreamerImage() : pipeline_(nullptr)
{
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
}

GStreamerImage::~GStreamerImage()
{
    destroyed();
}

void GStreamerImage::updateImage(const std::string image_path)
{
    // Destroy any existing pipeline
    destroyed();

    // 指定输出设备 device=/dev/fb0  指定宽高 video/x-raw,width=800,height=480
    std::cout << "显示图片: " << image_path << std::endl;
    // std::string pipelineStr = "filesrc location=\"" + image_path + "\" ! decodebin ! imagefreeze ! videoconvert ! fbdevsink"; // 适用于无头系统：kmssink 、fbdevsink

    // Build pipeline string
    std::string pipeline_str =
        "filesrc location=" + image_path +
        " ! decodebin ! imagefreeze"
        " ! videoconvert ! video/x-raw,format=RGB16"
        " ! fbdevsink device=/dev/fb0";

    // Create new pipeline
    GError *error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);

    if (error)
    {
        std::cerr << "Failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    if (!pipeline_)
    {
        std::cerr << "Failed to create pipeline" << std::endl;
        return;
    }

    // Start playback
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    // Wait for pipeline to initialize
    GstState state;
    gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);
    if (state != GST_STATE_PLAYING)
    {
        std::cerr << "Failed to start pipeline" << std::endl;
        destroyed();
    }
}

void GStreamerImage::destroyed()
{
    if (pipeline_)
    {
        // Stop pipeline
        gst_element_set_state(pipeline_, GST_STATE_NULL);

        // Release resources
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}