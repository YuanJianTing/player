#ifndef GST_PLAYER_H
#define GST_PLAYER_H

#include <gst/gst.h>
#include <string>
#include <vector>
#include <functional>
#include <gst/video/videooverlay.h>

struct VideoDisplaySettings
{
    int x = 0;      // 显示区域左上角X坐标
    int y = 0;      // 显示区域左上角Y坐标
    int width = 0;  // 显示宽度（0表示自动）
    int height = 0; // 显示高度（0表示自动）
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

    // 回调函数类型
    // using DurationCallback = std::function<void(int64_t duration)>;
    // using PositionCallback = std::function<void(int64_t position)>;
    using EosCallback = std::function<void()>;

    GstPlayer();
    ~GstPlayer();

    // 控制接口
    void play();
    void pause();
    void stop();
    State getState() const { return state_; }

    // 播放源管理

    void add_uri(const std::string &uri);
    void set_playlist(const std::vector<std::string> &uris, bool loop);
    void clear_list();

    // 进度控制
    void seek(int64_t position_ns);
    int64_t get_duration() const;
    int64_t get_position() const;

    // 回调设置
    void set_eos_callback(EosCallback cb);

    void set_uri(const std::string &uri);
    void set_video_display(const VideoDisplaySettings &settings);

private:
    GstElement *pipeline_ = nullptr;
    GstElement *playbin_ = nullptr;
    std::vector<std::string> playlist_;
    size_t current_uri_index_ = 0;
    bool loop_playlist_ = true;
    State state_ = State::STOPPED;

    GMainLoop *main_loop_;
    GThread *main_loop_thread_;

    // 回调函数
    EosCallback eos_cb_;
    GstElement *video_sink_ = nullptr; // 独立的视频渲染器实例
    VideoDisplaySettings display_settings_;

    // GStreamer 总线消息处理
    static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data);
    void handle_message(GstMessage *msg);

    // 初始化 GStreamer
    void initialize();
    void set_window_size(const int x, const int y, const int width, const int height);
    void cleanup();
};

#endif // GST_PLAYER_H