#ifndef GST_PLAYER_H
#define GST_PLAYER_H

#include <gst/gst.h>
#include <string>
#include <vector>
#include <functional>
#include <gst/video/videooverlay.h>

// 新增视频显示控制结构体
struct VideoDisplaySettings
{
    int x = 0;                               // 显示区域左上角X坐标
    int y = 0;                               // 显示区域左上角Y坐标
    int width = 0;                           // 显示宽度（0表示自动）
    int height = 0;                          // 显示高度（0表示自动）
    double alpha = 1.0;                      // 透明度 [0.0-1.0]
    bool force_aspect_ratio = true;          // 保持宽高比
    std::string sink_type = "autovideosink"; // 渲染器类型
};

class GstPlayer
{
public:
    // 播放器状态
    enum class State
    {
        PLAYING,
        PAUSED,
        STOPPED
    };

    // 新增：视频渲染器类型
    enum class VideoSinkType
    {
        AUTOVIDEOSINK, // 自动选择
        FBDEVSINK,     // Framebuffer
        XVIMAGESINK,   // X11
        KMSSINK        // DRM/KMS
    };

    // 回调函数类型
    using DurationCallback = std::function<void(int64_t duration)>;
    using PositionCallback = std::function<void(int64_t position)>;
    using EosCallback = std::function<void()>;

    GstPlayer();
    ~GstPlayer();

    // 控制接口
    void play();
    void pause();
    void stop();
    State getState() const { return state_; }

    // 播放源管理
    void set_uri(const std::string &uri);
    void add_uri(const std::string &uri);
    void set_playlist(const std::vector<std::string> &uris, bool loop);
    void clear_list();

    // 进度控制
    void seek(int64_t position_ns);
    int64_t get_duration() const;
    int64_t get_position() const;

    // 回调设置
    void set_duration_callback(DurationCallback cb);
    void set_position_callback(PositionCallback cb);
    void set_eos_callback(EosCallback cb);

    // 配置显示接口
    void set_video_sink_type(VideoSinkType type);
    void enable_audio(bool enable);

    // 视频显示控制接口
    void set_video_display(const VideoDisplaySettings &settings);
    void reset_video_display(); // 重置为默认显示

private:
    GstElement *pipeline_ = nullptr;
    GstElement *playbin_ = nullptr;
    std::vector<std::string> playlist_;
    size_t current_uri_index_ = 0;
    bool loop_playlist_ = true;
    State state_ = State::STOPPED;

    // 回调函数
    DurationCallback duration_cb_;
    PositionCallback position_cb_;
    EosCallback eos_cb_;

    // 渲染器
    VideoSinkType video_sink_type_ = VideoSinkType::KMSSINK; // KMSSINK  FBDEVSINK
    bool audio_enabled_ = false;

    VideoDisplaySettings display_settings_;
    GstElement *video_sink_ = nullptr; // 独立的视频渲染器实例

    // GStreamer 总线消息处理
    static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data);
    void handle_message(GstMessage *msg);

    // 初始化 GStreamer
    void initialize();
    void setup_sinks(); // 重新配置视频渲染器
    void cleanup();
};

#endif // GST_PLAYER_H