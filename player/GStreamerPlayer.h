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

private:
    struct Impl;
    Impl* impl;
};

#endif // GSTREAMER_PLAYER_H