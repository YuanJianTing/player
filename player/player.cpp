// player.cpp: 定义应用程序的入口点。
//

#include "player.h"
#include "GStreamerPlayer.h"
#include <thread>
#include <iostream>
#include <chrono>
#include <gst/gstclock.h>
#include <filesystem>

using namespace std;

int main(int argc, char* argv[])
{
	cout << "Hello CMake." << endl;

#ifdef _WIN32
    // 默认视频路径
    std::string defaultVideoPath = "C:/Users/yuan/Videos/fl.MOV";//fl.MOV 001_1734408210500.mp4

    //putenv("GST_DEBUG=3");  // 设置中等详细级别的调试输出


#else
    std::string defaultVideoPath = "/video/fl.mov";//fl.mov  001_1734408210500.mp4
#endif
    /*if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video file>" << std::endl;
        return 1;
    }*/

    GStreamerPlayer player;

    player.setErrorCallback([](GStreamerPlayer::ErrorType type, const std::string& msg) {
        std::cerr << "Error (" << static_cast<int>(type) << "): " << msg << std::endl;
        });

    player.setStateChangedCallback([](GStreamerPlayer::State state) {
        const char* states[] = { "NULL", "READY", "PAUSED", "PLAYING" };
        std::cout << "State changed to: " << states[static_cast<int>(state)] << std::endl;
        });

    player.setEOSCallback([]() {
        std::cout << "End of stream reached" << std::endl;
        //player->seek(0);
        });

   /* if (!player.load(argv[1])) {
        return 1;
    }*/

    if (!player.load(defaultVideoPath)) {
        return 1;
    }

    std::cout << "Duration: " << player.getDuration() / GST_MSECOND << " ms" << std::endl;

    std::cout << "执行play " << std::endl;
    if (!player.play()) {
        return 1;
    }

    std::cout << "play 执行完成 " << std::endl;

    // 主播放循环
    //bool playbackSuccess = true;
    //auto startTime = std::chrono::steady_clock::now();

    // 简单控制循环
    //while (true) {
    //    auto currentTime = std::chrono::steady_clock::now();
    //    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

    //    // 获取当前状态
    //    auto state = player.getCurrentState();
    //    auto position = player.getCurrentPosition() / GST_MSECOND;
    //    auto duration = player.getDuration() / GST_MSECOND;

    //    // 状态输出
    //   /* const char* stateNames[] = { "NULL", "READY", "PAUSED", "PLAYING", "ERROR" };
    //    std::cout << "[" << elapsed << "ms] State: " << stateNames[static_cast<int>(state)]
    //        << ", Pos: " << position << "ms/" << duration << "ms" << std::endl;*/

    //    // 异常状态处理
    //    if (state == GStreamerPlayer::State::NULL_ ||
    //        state == GStreamerPlayer::State::ERROR) {
    //        std::cerr << "Playback failed due to unexpected state" << std::endl;
    //        playbackSuccess = false;
    //        break;
    //    }

    //    // 正常结束检查
    //    if (duration > 0 && position >= duration - 100) { // 100ms提前量
    //        std::cout << "Playback completed successfully" << std::endl;

    //        //player.seek(0);
    //        //continue;
    //        //break;
    //    }

    //    // 超时检查（最长等待视频时长+10秒）
    //    if (duration > 0 && elapsed > (duration + 10000)) {
    //        std::cerr << "Playback timeout" << std::endl;
    //        playbackSuccess = false;
    //        break;
    //    }

    //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //}

    

    return 0;
	//return 0;
}
