#ifndef GSTREAMER_PLAYER_H
#define GSTREAMER_PLAYER_H

#include <string>
#include <functional>

class GStreamerPlayer {
public:
    enum class State { NULL_, READY, PAUSED, PLAYING, ERROR};
    enum class ErrorType { FILE_NOT_FOUND, PIPELINE_FAILURE, DECODE_FAILURE };

    using ErrorCallback = std::function<void(ErrorType, const std::string&)>;
    using StateChangedCallback = std::function<void(State)>;
    using EOSCallback = std::function<void()>;

    GStreamerPlayer();
    ~GStreamerPlayer();

    bool load(const std::string& filePath);
    bool play();
    bool pause();
    bool stop();
    bool seek(int64_t nanoseconds);

    void setErrorCallback(ErrorCallback callback);
    void setStateChangedCallback(StateChangedCallback callback);
    void setEOSCallback(EOSCallback callback);

    State getCurrentState() const;
    int64_t getDuration() const;
    int64_t getCurrentPosition() const;

    /// <summary>
    /// 更新显示图片
    /// </summary>
    /// <param name="imagePath"></param>
    void setOverlayImage(const std::string& imagePath);

    /// <summary>
    /// 更新视频位置
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    void setVideoPosition(int x,int y);
    /// <summary>
    ///更新图片位置
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    void setOverlayPosition(int x, int y);
    /// <summary>
    /// 设置输出尺寸
    /// </summary>
    /// <param name="width"></param>
    /// <param name="height"></param>
    void setOutputSize(int width, int height);

private:
    struct Impl;
    Impl* impl;
};

#endif // GSTREAMER_PLAYER_H