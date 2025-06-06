// GstConcatPlayer.h
#pragma once
#include <gst/gst.h>
#include <vector>
#include <string>
#include <functional>

class GstConcatPlayer
{
public:
    GstConcatPlayer();
    ~GstConcatPlayer();

    void add_uri(const std::string &uri);
    void play();
    void stop();
    void set_window_size(int x, int y, int width, int height);
    void set_eos_callback(std::function<void()> cb);

private:
    void create_pipeline();
    static void on_pad_added(GstElement *decodebin, GstPad *pad, gpointer user_data);
    static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer user_data);
    void link_decodebin_to_concat(GstElement *decodebin);
    void seek_to_start();

private:
    GstElement *pipeline_ = nullptr;
    GstElement *concat_ = nullptr;
    GstElement *video_convert_ = nullptr;
    GstElement *video_sink_ = nullptr;

    std::vector<std::string> uris_;
    std::function<void()> eos_cb_;
};
