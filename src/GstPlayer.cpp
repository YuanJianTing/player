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

void GstPlayer::setup_sinks()
{
    // 视频渲染器
    GstElement *video_sink = nullptr;
    const char *sink_name = nullptr;
    switch (video_sink_type_)
    {
    case VideoSinkType::FBDEVSINK:
        sink_name = "fbdevsink";
        break;
    case VideoSinkType::XVIMAGESINK:
        sink_name = "ximagesink";
        break;
    case VideoSinkType::KMSSINK:
        sink_name = "kmssink";
        break;
    default:
        break;
    }

    if (sink_name)
    {
        video_sink = gst_element_factory_make(sink_name, nullptr);
        if (!video_sink)
        {
            std::cerr << "WARNING: " << sink_name << " not available, using default" << std::endl;
        }
        else
        {

            std::cout << "视频皮肤：---------- " << sink_name << std::endl;

            GstElementFactory *factory = gst_element_get_factory(video_sink_);
            bool is_supported = gst_element_factory_has_interface(factory, "GstVideoOverlay");
            std::cout << "检查渲染器是否支持位置控制:" << is_supported << std::endl;

            if (is_supported)
            {
                std::cout << "设置视频位置----------" << std::endl;

                GstVideoOverlay *overlay = GST_VIDEO_OVERLAY(video_sink_);
                gst_video_overlay_set_render_rectangle(overlay,
                                                       0, 0,
                                                       800, 1280);

                // GstElement *videobox = gst_element_factory_make("videobox", "position_filter");
                // g_object_set(videobox,
                //              "border-alpha", 0,
                //              "left", 0,
                //              "top", 0,
                //              "right", 800,
                //              "bottom", 1280,
                //              nullptr);

                // // 动态插入到pipeline中
                // GstElement *queue = gst_element_factory_make("queue", nullptr);
                // gst_bin_add_many(GST_BIN(playbin_), queue, videobox, nullptr);
                // gst_element_link_many(queue, videobox, video_sink_, nullptr);
            }

            // std::string rectangle = "0,0,800,1280";
            // g_object_set(video_sink_,
            //              "render-rectangle", rectangle.c_str(),
            //              nullptr);

            g_object_set(playbin_, "video-sink", video_sink, nullptr);
        }
    }

    // 音频处理
    if (!audio_enabled_)
    {
        GstElement *fake_audio = gst_element_factory_make("fakesink", nullptr);
        g_object_set(playbin_, "audio-sink", fake_audio, nullptr);
    }

    // switch (video_sink_type_)
    // {
    // case VideoSinkType::FBDEVSINK:
    //     video_sink = gst_element_factory_make("fbdevsink", "fbdevsink");
    //     break;
    // case VideoSinkType::XVIMAGESINK:
    //     video_sink = gst_element_factory_make("ximagesink", "ximagesink");
    //     break;
    // case VideoSinkType::KMSSINK:
    //     video_sink = gst_element_factory_make("kmssink", "kmssink");
    //     break;
    // default:
    //     // 使用 playbin 默认选择
    //     break;
    // }

    // if (video_sink)
    // {
    //     g_object_set(playbin_, "video-sink", video_sink, nullptr);
    // }

    // // 设置音频
    // if (!audio_enabled_)
    // {
    //     GstElement *fake_audio = gst_element_factory_make("fakesink", "fakeaudio");
    //     g_object_set(playbin_, "audio-sink", fake_audio, nullptr);
    // }
}

void GstPlayer::initialize()
{

    playbin_ = gst_element_factory_make("playbin", "player");
    if (!playbin_)
    {
        std::cerr << "Failed to create playbin" << std::endl;
        return;
    }

    // 初始设置 Sink
    // setup_sinks();

    // 设置总线监听
    GstBus *bus = gst_element_get_bus(playbin_);
    gst_bus_add_watch(bus, bus_callback, this);
    gst_object_unref(bus);
}

void GstPlayer::cleanup()
{
    if (playbin_)
    {
        gst_element_set_state(playbin_, GST_STATE_NULL);
        gst_object_unref(playbin_);
        playbin_ = nullptr;
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
    current_uri_index_ = 0;

    std::cout << "设置播放器皮肤" << std::endl;

    GValue render_rectangle = G_VALUE_INIT;
    g_value_init(&render_rectangle, GST_TYPE_ARRAY);
    // 设置位置及大小

    int x = 0;
    int y = 0;
    int width = 1280;
    int height = 720;
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

    g_object_set(video_sink_, "can-scale", true, NULL);

    g_object_set(playbin_, "video-sink", video_sink_, NULL);

    // g_object_set(playbin_, "can-scale", video_sink_, NULL);
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
    GstPlayer *player = static_cast<GstPlayer *>(data);
    player->handle_message(msg);
    return TRUE;
}

void GstPlayer::handle_message(GstMessage *msg)
{
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        if (eos_cb_)
            eos_cb_();
        if (!playlist_.empty() && ++current_uri_index_ < playlist_.size())
        {
            set_uri(playlist_[current_uri_index_]);
            play();
        }
        else if (loop_playlist_ && !playlist_.empty())
        {
            current_uri_index_ = 0;
            set_uri(playlist_[0]);
            play();
        }
        else
        {
            state_ = State::STOPPED;
        }
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        if (duration_cb_)
            duration_cb_(get_duration());
        break;
    case GST_MESSAGE_STATE_CHANGED:
        if (position_cb_)
            position_cb_(get_position());
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
}

void GstPlayer::set_video_sink_type(VideoSinkType type)
{
    video_sink_type_ = type;
    if (playbin_)
    {
        setup_sinks();
    }
}
void GstPlayer::enable_audio(bool enable)
{
    audio_enabled_ = enable;
    if (playbin_)
    {
        setup_sinks();
    }
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

    setup_sinks();
}

void GstPlayer::reset_video_display()
{
    set_video_display({.x = 0,
                       .y = 0,
                       .width = 0,
                       .height = 0,
                       .alpha = 1.0,
                       .force_aspect_ratio = true,
                       .sink_type = "autovideosink"});
}

// 回调设置
void GstPlayer::set_duration_callback(DurationCallback cb)
{
    duration_cb_ = cb;
}

void GstPlayer::set_position_callback(PositionCallback cb)
{
    position_cb_ = cb;
}

void GstPlayer::set_eos_callback(EosCallback cb)
{
    eos_cb_ = cb;
}