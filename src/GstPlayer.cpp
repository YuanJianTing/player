#include "GstPlayer.h"
#include <iostream>
#include "gst/play/play.h"
#include <string>
#include <gst/video/video.h>
#include <gst/gst.h>

GstPlayer::GstPlayer()
{
    gst_init(nullptr, nullptr);
    initialize();
}

GstPlayer::~GstPlayer()
{
    cleanup();
}

void GstPlayer::initialize()
{

    playbin_ = gst_element_factory_make("playbin", "player");
    if (!playbin_)
    {
        std::cerr << "Failed to create playbin" << std::endl;
        return;
    }
    // 初始化主循环
    main_loop_ = g_main_loop_new(nullptr, FALSE);

    // 启动主循环线程
    main_loop_thread_ = g_thread_new("gst-main-loop", [](gpointer data) -> gpointer
                                     {
            GstPlayer* player = static_cast<GstPlayer*>(data);
            g_main_loop_run(player->main_loop_);
            return nullptr; }, this);

    // 设置总线监听
    GstBus *bus = gst_element_get_bus(playbin_);
    gst_bus_add_watch(bus, bus_callback, this);
    gst_object_unref(bus);
}

void GstPlayer::set_window_size(const int x, const int y, const int width, const int height)
{
    std::cout << "设置窗口大小：" << x << "," << y << "," << width << "," << height << std::endl;

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
    g_object_set(video_sink_, "can-scale", false, NULL);
    g_object_set(playbin_, "video-sink", video_sink_, NULL);

    gst_object_unref(video_sink_);
    video_sink_ = nullptr;
}

void GstPlayer::cleanup()
{
    if (main_loop_)
    {
        g_main_loop_quit(main_loop_);
        g_thread_join(main_loop_thread_);
        g_main_loop_unref(main_loop_);
        main_loop_ = nullptr;
    }
    if (playbin_)
    {
        gst_element_set_state(playbin_, GST_STATE_NULL);
        gst_object_unref(playbin_);
        playbin_ = nullptr;
    }
    if (video_sink_)
    {
        gst_object_unref(video_sink_);
        video_sink_ = nullptr;
    }
}

// 播放控制
void GstPlayer::play()
{
    if (!playbin_)
        return;
    gst_element_set_state(playbin_, GST_STATE_PLAYING);
    state_ = State::PLAYING;
}

void GstPlayer::pause()
{
    if (!playbin_)
        return;
    gst_element_set_state(playbin_, GST_STATE_PAUSED);
    state_ = State::PAUSED;
}

void GstPlayer::stop()
{
    if (!playbin_)
        return;
    gst_element_set_state(playbin_, GST_STATE_NULL);
    state_ = State::STOPPED;
}

// 设置播放源
void GstPlayer::set_uri(const std::string &uri)
{
    if (!playbin_)
        return;
    g_object_set(playbin_, "uri", uri.c_str(), nullptr);
    // 确保每次切换视频都设置窗口
    set_window_size(display_settings_.x, display_settings_.y, display_settings_.width, display_settings_.height);
}
void GstPlayer::add_uri(const std::string &uri)
{
    if (!playbin_)
    {
        std::cout << "播放器没有准备好" << std::endl;
        return;
    }
    // 如果播放列表为空，则立即播放当前内容
    if (playlist_.empty())
    {
        std::cout << "设置播放源" << std::endl;
        set_uri(uri);
    }
    playlist_.push_back(uri);
}
void GstPlayer::set_playlist(const std::vector<std::string> &uris, bool loop)
{
    playlist_ = uris;
    loop_playlist_ = loop;
    current_uri_index_ = 0;
    if (!playlist_.empty())
    {
        current_uri_index_ = 0;
        set_uri(playlist_[0]);
    }
}
void GstPlayer::clear_list()
{
    current_uri_index_ = 0;
    playlist_.clear();
}

// 进度控制
void GstPlayer::seek(int64_t position_ns)
{
    if (!playbin_)
        return;
    gst_element_seek_simple(
        playbin_,
        GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH,
        position_ns);
}

int64_t GstPlayer::get_duration() const
{
    if (!playbin_)
        return 0;
    gint64 duration = 0;
    gst_element_query_duration(playbin_, GST_FORMAT_TIME, &duration);
    return duration;
}

int64_t GstPlayer::get_position() const
{
    if (!playbin_)
        return 0;
    gint64 position = 0;
    gst_element_query_position(playbin_, GST_FORMAT_TIME, &position);
    return position;
}

// 总线消息处理
gboolean GstPlayer::bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
    // 打印消息类型用于调试
    // std::cout << "Received message: " << GST_MESSAGE_TYPE_NAME(msg) << std::endl;
    GstPlayer *player = static_cast<GstPlayer *>(data);
    // 确保在主线程中处理消息
    g_idle_add([](gpointer data) -> gboolean
               {
        auto* msg_data = static_cast<std::pair<GstPlayer*, GstMessage*>*>(data);
        msg_data->first->handle_message(msg_data->second);
        delete msg_data;
        return G_SOURCE_REMOVE; }, new std::pair<GstPlayer *, GstMessage *>(player, gst_message_ref(msg)));

    return TRUE;
}

void GstPlayer::handle_message(GstMessage *msg)
{
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        // state_ = State::STOPPED;
        //  先停止并重置管道
        gst_element_set_state(playbin_, GST_STATE_READY);

        if (!playlist_.empty() && ++current_uri_index_ < playlist_.size())
        {
            std::cout << "切换下一个视频" << std::endl;

            set_uri(playlist_[current_uri_index_]);
            play();
        }
        else if (loop_playlist_ && !playlist_.empty())
        {
            std::cout << "回到第一个视频" << std::endl;
            current_uri_index_ = 0;
            set_uri(playlist_[current_uri_index_]);
            play();
        }
        else
        {
            std::cout << "播放列表为空，结束播放" << std::endl;
            state_ = State::STOPPED;
        }

        if (eos_cb_)
            eos_cb_();
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        // if (duration_cb_)
        //     duration_cb_(get_duration());
        break;
    case GST_MESSAGE_STATE_CHANGED:
        // if (position_cb_)
        //     position_cb_(get_position());
        break;
    case GST_MESSAGE_ERROR:
        gchar *debug;
        GError *err;
        gst_message_parse_error(msg, &err, &debug);
        std::cerr << "Error: " << err->message << std::endl;
        g_error_free(err);
        g_free(debug);
        break;
    default:
        break;
    }
    gst_message_unref(msg);
}

// 回调设置
void GstPlayer::set_eos_callback(EosCallback cb)
{
    eos_cb_ = cb;
}

void GstPlayer::set_video_display(const VideoDisplaySettings &settings)
{
    display_settings_ = settings;

    // 自动处理零宽高（保持视频原始尺寸）
    if (settings.width <= 0 || settings.height <= 0)
    {
        gint64 duration = 0;
        gst_element_query_duration(playbin_, GST_FORMAT_TIME, &duration);
        if (duration > 0)
        {
            GstPad *pad = gst_element_get_static_pad(video_sink_, "sink");
            GstCaps *caps = gst_pad_get_current_caps(pad);
            if (caps)
            {
                GstVideoInfo info;
                gst_video_info_from_caps(&info, caps);
                display_settings_.width = info.width;
                display_settings_.height = info.height;
                gst_caps_unref(caps);
            }
            gst_object_unref(pad);
        }
    }

    set_window_size(display_settings_.x, display_settings_.y, display_settings_.width, display_settings_.height);
}