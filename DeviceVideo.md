以下是完整的 **视频显示位置控制** 实现方案，通过修改 `GstPlayer` 类支持动态调整视频显示区域（位置、大小、旋转等）：

---

### **1. 头文件扩展（GstPlayer.h）**
```cpp
// 新增视频显示控制结构体
struct VideoDisplaySettings {
    int x = 0;              // 显示区域左上角X坐标
    int y = 0;              // 显示区域左上角Y坐标
    int width = 0;          // 显示宽度（0表示自动）
    int height = 0;         // 显示高度（0表示自动）
    double alpha = 1.0;     // 透明度 [0.0-1.0]
    bool force_aspect_ratio = true; // 保持宽高比
    std::string sink_type = "autovideosink"; // 渲染器类型
};

class GstPlayer {
public:
    // ... 原有代码 ...

    // 视频显示控制接口
    void set_video_display(const VideoDisplaySettings& settings);
    void reset_video_display(); // 重置为默认显示

private:
    VideoDisplaySettings display_settings_;
    GstElement* video_sink_ = nullptr; // 独立的视频渲染器实例

    void setup_video_sink(); // 重新配置视频渲染器
};
```

---

### **2. 核心实现（GstPlayer.cpp）**
#### **2.1 视频渲染器动态配置**
```cpp
void GstPlayer::setup_video_sink() {
    if (!playbin_) return;

    // 销毁旧渲染器
    if (video_sink_) {
        gst_element_set_state(video_sink_, GST_STATE_NULL);
        gst_object_unref(video_sink_);
    }

    // 创建新渲染器
    video_sink_ = gst_element_factory_make(display_settings_.sink_type.c_str(), "custom_sink");
    if (!video_sink_) {
        std::cerr << "Failed to create video sink: " << display_settings_.sink_type << std::endl;
        return;
    }

    // 通用属性设置
    g_object_set(video_sink_,
        "force-aspect-ratio", display_settings_.force_aspect_ratio,
        "alpha", display_settings_.alpha,
        nullptr);

    // 特殊渲染器配置（如ximagesink/kmssink）
    if (display_settings_.sink_type == "ximagesink") {
        g_object_set(video_sink_,
            "x", display_settings_.x,
            "y", display_settings_.y,
            nullptr);
    } else if (display_settings_.sink_type == "kmssink") {
        g_object_set(video_sink_,
            "render-rectangle", gst_video_rectangle_new(
                display_settings_.x,
                display_settings_.y,
                display_settings_.width,
                display_settings_.height),
            nullptr);
    }

    // 链接到playbin
    g_object_set(playbin_, "video-sink", video_sink_, nullptr);
}
```

#### **2.2 显示控制接口**
```cpp
void GstPlayer::set_video_display(const VideoDisplaySettings& settings) {
    display_settings_ = settings;
    
    // 自动处理零宽高（保持视频原始尺寸）
    if (settings.width <= 0 || settings.height <= 0) {
        gint64 duration = 0;
        gst_element_query_duration(playbin_, GST_FORMAT_TIME, &duration);
        if (duration > 0) {
            GstPad* pad = gst_element_get_static_pad(video_sink_, "sink");
            GstCaps* caps = gst_pad_get_current_caps(pad);
            if (caps) {
                GstVideoInfo info;
                gst_video_info_from_caps(&info, caps);
                display_settings_.width = info.width;
                display_settings_.height = info.height;
                gst_caps_unref(caps);
            }
            gst_object_unref(pad);
        }
    }

    setup_video_sink();
}

void GstPlayer::reset_video_display() {
    set_video_display({
        .x = 0,
        .y = 0,
        .width = 0,
        .height = 0,
        .alpha = 1.0,
        .force_aspect_ratio = true,
        .sink_type = "autovideosink"
    });
}
```

---

### **3. 使用示例**
#### **3.1 基础位置控制**
```cpp
GstPlayer player;
player.set_uri("file:///path/to/video.mp4");

// 在(100,50)位置显示，大小设为300x200
player.set_video_display({
    .x = 100,
    .y = 50,
    .width = 300,
    .height = 200,
    .sink_type = "ximagesink" // 需要X11环境
});

player.play();
```

#### **3.2 嵌入式设备（DRM/KMS）**
```cpp
// 在DRM设备上全屏显示右上角1/4区域
player.set_video_display({
    .x = 1920/2, // 假设屏幕宽度1920
    .y = 0,
    .width = 1920/2,
    .height = 1080/2,
    .sink_type = "kmssink"
});
```

#### **3.3 透明叠加显示**
```cpp
// 半透明显示（需要支持透明的渲染器）
player.set_video_display({
    .alpha = 0.5,
    .sink_type = "gtksink" // 或 "waylandsink"
});
```

---

### **4. 兼容性处理**
#### **4.1 自动回退机制**
```cpp
void GstPlayer::setup_video_sink() {
    // ... 原有代码 ...

    if (!gst_element_link(playbin_, video_sink_)) {
        std::cerr << "Failed to link video sink, falling back to default" << std::endl;
        display_settings_.sink_type = "autovideosink";
        video_sink_ = nullptr;
        g_object_set(playbin_, "video-sink", nullptr, nullptr); // 恢复默认
    }
}
```

#### **4.2 渲染器能力检测**
```cpp
bool GstPlayer::is_sink_supported(const std::string& sink_type) {
    GstElementFactory* factory = gst_element_factory_find(sink_type.c_str());
    if (factory) {
        gst_object_unref(factory);
        return true;
    }
    return false;
}
```

---

### **5. 高级功能扩展**
#### **5.1 动态调整位置**
```cpp
// 实时移动视频窗口（例如实现画中画）
void update_position(GstPlayer& player, int x, int y) {
    auto settings = player.get_display_settings(); // 需添加getter
    settings.x = x;
    settings.y = y;
    player.set_video_display(settings);
}
```

#### **5.2 旋转与镜像**
```cpp
// 需要视频过滤器支持
void GstPlayer::set_video_rotation(int degrees) {
    GstElement* rotate = gst_element_factory_make("videoflip", nullptr);
    g_object_set(rotate, "method", degrees / 90, nullptr);
    
    // 插入到pipeline中
    gst_bin_add(GST_BIN(playbin_), rotate);
    gst_element_link_many(video_sink_, rotate, nullptr);
}
```

---

### **6. 各平台支持情况**
| 渲染器类型      | 位置控制 | 透明度 | 适用平台               |
|----------------|----------|--------|-----------------------|
| **ximagesink** | ✅        | ❌      | Linux (X11)           |
| **kmssink**    | ✅        | ❌      | 嵌入式Linux (DRM)     |
| **waylandsink**| ✅        | ✅      | Wayland桌面           |
| **gtksink**    | ✅        | ✅      | 跨平台（需要GTK）     |
| **fbdevsink**  | ❌        | ❌      | 老旧Linux帧缓冲       |

---

### **7. 完整CMake配置**
```cmake
# 确保包含视频和显示相关插件
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-video-1.0
    gstreamer-plugins-base-1.0  # 基础渲染器
    gstreamer-plugins-good-1.0  # ximagesink
    gstreamer-plugins-bad-1.0   # kmssink/waylandsink
)

target_link_libraries(your_target PRIVATE ${GST_LIBRARIES})
```

---

通过这种设计，您可以：
1. **精确控制**视频在屏幕上的位置和大小
2. **动态调整**显示参数（如实时移动窗口）
3. **自动适配**不同渲染后端（X11/Wayland/DRM）
4. **优雅降级**在不支持某些功能的平台上自动回退