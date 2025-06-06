// GstConcatPlayer.cpp
#include "GstConcatPlayer.h"
#include <iostream>
#include <gst/gst.h>
#include <vector>
#include <string>

GstConcatPlayer::GstConcatPlayer()
{
    gst_init(nullptr, nullptr);
    create_pipeline();
}

GstConcatPlayer::~GstConcatPlayer()
{
    if (pipeline_)
    {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
    }
}

void GstConcatPlayer::create_pipeline()
{
    pipeline_ = gst_pipeline_new("video-concat-pipeline");
    concat_ = gst_element_factory_make("concat", "concat");
    gst_bin_add(GST_BIN(pipeline_), concat_);

    // 为每个 URI 添加 uridecodebin
    for (const auto &uri : uris_)
    {
        GstElement *decodebin = gst_element_factory_make("uridecodebin", nullptr);
        g_object_set(decodebin, "uri", uri.c_str(), nullptr);
        g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), this);
        gst_bin_add(GST_BIN(pipeline_), decodebin);
    }

    // concat -> videoconvert -> kmssink
    GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    video_sink_ = gst_element_factory_make("kmssink", "videosink");
    gst_bin_add_many(GST_BIN(pipeline_), videoconvert, video_sink_, nullptr);
    gst_element_link(concat_, videoconvert);
    gst_element_link(videoconvert, video_sink_);

    // 总线
    GstBus *bus = gst_element_get_bus(pipeline_);
    gst_bus_add_watch(bus, bus_callback, this);
    gst_object_unref(bus);
}

gboolean GstConcatPlayer::bus_callback(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    auto *player = static_cast<GstConcatPlayer *>(user_data);
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        // 播放结束，seek到开头，循环播放
        g_print("End of stream, looping\n");
        player->stop();
        player->create_pipeline();
        player->play();
        // player->seek_to_start();
        break;
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *debug;
        gst_message_parse_error(msg, &err, &debug);
        g_printerr("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

void GstConcatPlayer::add_uri(const std::string &uri)
{
    uris_.push_back(uri);

    auto *decodebin = gst_element_factory_make("uridecodebin", nullptr);
    g_object_set(decodebin, "uri", uri.c_str(), nullptr);

    g_signal_connect(decodebin, "pad-added", G_CALLBACK(GstConcatPlayer::on_pad_added), this);

    gst_bin_add(GST_BIN(pipeline_), decodebin);
    gst_element_sync_state_with_parent(decodebin);
}

void GstConcatPlayer::on_pad_added(GstElement *decodebin, GstPad *pad, gpointer user_data)
{
    GstCaps *caps = gst_pad_get_current_caps(pad);
    const gchar *name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    gst_caps_unref(caps);

    if (g_str_has_prefix(name, "video/"))
    {
        auto *player = static_cast<GstConcatPlayer *>(user_data);

        GstElement *queue = gst_element_factory_make("queue", nullptr);
        gst_bin_add(GST_BIN(player->pipeline_), queue);
        gst_element_sync_state_with_parent(queue);

        GstPad *queue_sink_pad = gst_element_get_static_pad(queue, "sink");
        gst_pad_link(pad, queue_sink_pad);
        gst_object_unref(queue_sink_pad);

        GstPad *concat_sink_pad = gst_element_get_request_pad(player->concat_, "sink_%u");
        GstPad *queue_src_pad = gst_element_get_static_pad(queue, "src");
        gst_pad_link(queue_src_pad, concat_sink_pad);
        gst_object_unref(queue_src_pad);
        gst_object_unref(concat_sink_pad);
    }

    // auto *player = static_cast<GstConcatPlayer *>(user_data);
    // GstCaps *caps = gst_pad_get_current_caps(pad);
    // if (!caps)
    // {
    //     caps = gst_pad_query_caps(pad, nullptr);
    // }
    // if (!caps)
    // {
    //     std::cerr << "No caps found on pad, skipping." << std::endl;
    //     return;
    // }

    // const gchar *name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    // gst_caps_unref(caps);

    // if (g_str_has_prefix(name, "video/"))
    // {
    //     // 创建一个 queue 以避免阻塞
    //     GstElement *queue = gst_element_factory_make("queue", nullptr);
    //     gst_bin_add(GST_BIN(player->pipeline_), queue);
    //     gst_element_sync_state_with_parent(queue);

    //     // 连接 decodebin pad -> queue sink pad
    //     GstPad *queue_sink_pad = gst_element_get_static_pad(queue, "sink");
    //     if (gst_pad_link(pad, queue_sink_pad) != GST_PAD_LINK_OK)
    //     {
    //         std::cerr << "Failed to link decodebin pad to queue" << std::endl;
    //         gst_object_unref(queue_sink_pad);
    //         return;
    //     }
    //     gst_object_unref(queue_sink_pad);

    //     // 从 concat 请求一个 sink pad
    //     GstPad *concat_sink_pad = gst_element_get_request_pad(player->concat_, "sink_%u");
    //     GstPad *queue_src_pad = gst_element_get_static_pad(queue, "src");
    //     if (gst_pad_link(queue_src_pad, concat_sink_pad) != GST_PAD_LINK_OK)
    //     {
    //         std::cerr << "Failed to link queue to concat" << std::endl;
    //     }
    //     gst_object_unref(queue_src_pad);
    //     gst_object_unref(concat_sink_pad);
    // }
}

void GstConcatPlayer::link_decodebin_to_concat(GstElement *decodebin)
{
    GstPad *src_pad = gst_element_get_static_pad(decodebin, "src");
    GstPad *sink_pad = gst_element_get_request_pad(concat_, "sink_%u");
    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK)
    {
        std::cerr << "Failed to link decodebin to concat" << std::endl;
    }
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
}

void GstConcatPlayer::play()
{
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

void GstConcatPlayer::stop()
{
    gst_element_set_state(pipeline_, GST_STATE_NULL);
}

void GstConcatPlayer::seek_to_start()
{
    // GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT
    gst_element_seek_simple(pipeline_,
                            GST_FORMAT_TIME,
                            GST_SEEK_FLAG_FLUSH,
                            0);
}

void GstConcatPlayer::set_window_size(int x, int y, int width, int height)
{
    // GValue render_rectangle = G_VALUE_INIT;
    // g_value_init(&render_rectangle, GST_TYPE_ARRAY);
    // for (int val : {x, y, width, height})
    // {
    //     GValue item = G_VALUE_INIT;
    //     g_value_init(&item, G_TYPE_INT);
    //     g_value_set_int(&item, val);
    //     gst_value_array_append_value(&render_rectangle, &item);
    // }
    // g_object_set(video_sink_, "render-rectangle", &render_rectangle, nullptr);
    // g_value_unset(&render_rectangle);

    GValue render_rectangle = G_VALUE_INIT;
    g_value_init(&render_rectangle, GST_TYPE_ARRAY);
    // 设置位置及大小
    for (int val : {x, y, width, height})
    {
        GValue item = G_VALUE_INIT;
        g_value_init(&item, G_TYPE_INT);
        g_value_set_int(&item, val);
        gst_value_array_append_value(&render_rectangle, &item);
    }

    const gchar *property_name = "render-rectangle";
    video_sink_ = gst_element_factory_make_with_properties("kmssink", 1, &property_name, &render_rectangle);
    g_value_unset(&render_rectangle);
}

void GstConcatPlayer::set_eos_callback(std::function<void()> cb)
{
    eos_cb_ = cb;
}
