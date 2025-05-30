#include "GStreamerPlayer.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>
#include <iostream>
#include <mutex>
#include <filesystem>

namespace fs = std::filesystem;

struct GStreamerPlayer::Impl {
    GstElement* pipeline = nullptr;
    GstElement* source = nullptr;
    GstElement* sink = nullptr;

    ErrorCallback errorCallback;
    StateChangedCallback stateCallback;
    EOSCallback eosCallback;

    State currentState = State::NULL_;
    std::mutex stateMutex;

    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
        auto self = static_cast<Impl*>(data);

        //std::cout << "busCallback��msg= " << msg->type << std::endl;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);

            std::string errorMsg = err ? err->message : "Unknown error";
            if (err) {
                std::cerr << "GStreamer Error: " << errorMsg << std::endl;
                g_error_free(err);
            }
            if (debug) {
                std::cerr << "Debug info: " << debug << std::endl;
                g_free(debug);
            }

            if (self->errorCallback) {
                self->errorCallback(ErrorType::PIPELINE_FAILURE, errorMsg);
            }
            break;
        }
        case GST_MESSAGE_EOS://��Ƶ�������ʱ
            //���¿�ʼ����
            std::cout << "���¿�ʼ����" << std::endl;
            // ʹ��g_idle_addȷ�������߳���������ִ��seek����
            //g_idle_add([](gpointer data) -> gboolean {
            //    auto pipeline = static_cast<GstElement*>(data);
            //    if (!gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
            //        static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0)) {
            //        std::cerr << "Seek to start failed!" << std::endl;
            //    }
            //    return G_SOURCE_REMOVE; // ִֻ��һ��
            //    }, gst_object_ref(self->pipeline)); // �������ü�����ֹ�ڴ�й©

            gst_element_seek_simple(self->pipeline, GST_FORMAT_TIME,
                static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);


            if (self->eosCallback) {
                self->eosCallback();
            }
            break;
        case GST_MESSAGE_STATE_CHANGED: {

            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(self->pipeline)) {
                GstState old_state, new_state, pending;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending);
                //std::cout << "״̬����� Old: " << old_state << ", New: " << new_state << std::endl;
                if (new_state == GST_STATE_PLAYING) {
                    std::cout << "Pipeline is playing" << std::endl;
                }

                State s = State::NULL_;
                switch (new_state) {
                case GST_STATE_READY: s = State::READY; break;
                case GST_STATE_PAUSED: s = State::PAUSED; break;
                case GST_STATE_PLAYING: s = State::PLAYING; break;
                default: break;
                }

                {
                    std::lock_guard<std::mutex> lock(self->stateMutex);
                    self->currentState = s;
                }

                if (self->stateCallback) {
                    self->stateCallback(s);
                }
            }

            break;
        }
        default:
            break;
        }

        return TRUE;
    }
};

GStreamerPlayer::GStreamerPlayer() : impl(new Impl()) {
    gst_init(nullptr, nullptr);
}

GStreamerPlayer::~GStreamerPlayer() {
    stop();
    delete impl;
}

bool GStreamerPlayer::load(const std::string& filePath) {
    stop();

    // 1. ����׳���ļ����
    try {
        if (!fs::exists(filePath)) {
            if (impl->errorCallback) {
                impl->errorCallback(ErrorType::FILE_NOT_FOUND,
                    "File not found: " + filePath);
            }
            return false;
        }
    }
    catch (const fs::filesystem_error& e) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::FILE_NOT_FOUND,
                "Filesystem error: " + std::string(e.what()));
        }
        return false;
    }

#ifdef _WIN32
    //std::string uri = "file:///" + filePath;
    //std::replace(uri.begin(), uri.end(), '\\', '/'); // Windows·��ת��

    std::string uri = filePath;
    // ���Զ��ֹܵ�����
    const char* pipeline_configs[] = {
        // "playbin uri=%s",  // ����1��ʹ��playbin
        // "uridecodebin uri=%s ! videoconvert ! autovideosink",  // ����2
        "filesrc location=%s ! decodebin  ! d3dvideosink"  // ����3
        // "filesrc location=%s ! decodebin  ! videoconvert ! autovideosink"  // ����3
    };

#else
     // 2. ʹ�ø��򵥵Ĺܵ�����
    //std::string uri = "file://" + filePath;
    std::string uri =  filePath;
    //std::replace(uri.begin(), uri.end(), '\\', '/'); // Windows·��ת��

    //gst-launch-1.0 filesrc location=/video/001_1734408210500.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! kmssink driver-name=vc4
    // ���Զ��ֹܵ�����
    const char* pipeline_configs[] = {
        "filesrc location=%s ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! kmssink driver-name=vc4",//��ݮ���豸
       // "playbin uri=%s",  // ����1��ʹ��playbin
       // "uridecodebin uri=%s ! videoconvert ! autovideosink",  // ����2
        //"filesrc location=%s ! qtdemux ! queue ! h264parse ! avdec_h264 ! videoconvert ! autovideosink"  // ����3
    };
#endif
   

    GError* error = nullptr;
    bool pipeline_created = false;

    for (size_t i = 0; i < sizeof(pipeline_configs) / sizeof(pipeline_configs[0]); ++i) {
        char* pipeline_str = g_strdup_printf(pipeline_configs[i], uri.c_str());

        std::cout << "������Ƶ��� " << pipeline_str << std::endl;

        impl->pipeline = gst_parse_launch(pipeline_str, &error);
        g_free(pipeline_str);

        if (impl->pipeline) {
            std::cout << "Pipeline created with config " << i << std::endl;
            pipeline_created = true;
            break;
        }

        if (error) {
            std::cerr << "Pipeline config " << i << " failed: " << error->message << std::endl;
            g_clear_error(&error);
        }
    }

    if (!pipeline_created) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE,
                "All pipeline configurations failed");
        }
        return false;
    }

    // 3. ����ȫ��bus����
    GstBus* bus = gst_element_get_bus(impl->pipeline);
    if (!bus) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE, "Failed to get pipeline bus");
        }
        return false;
    }

    if (!gst_bus_add_watch(bus, Impl::busCallback, impl)) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE, "Failed to add bus watch");
        }
        gst_object_unref(bus);
        return false;
    }
    gst_object_unref(bus);


    // 4. Ԥ���ػ�ȡ��Ϣ
    GstStateChangeReturn ret = gst_element_set_state(impl->pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE,
                "Failed to prepare pipeline (READY state)");
        }
        return false;
    }

    // �ȴ�״̬ת�����
    GstState state;
    if (gst_element_get_state(impl->pipeline, &state, nullptr, 5 * GST_SECOND) == GST_STATE_CHANGE_FAILURE) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE,
                "Timeout while preparing pipeline");
        }
        return false;
    }

    return true;
}

bool GStreamerPlayer::play() {
    if (!impl->pipeline) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE, "No valid pipeline to play");
        }
        return false;
    }

    // ȷ������PAUSED״̬�Ի�ȡʱ������Ϣ
    GstStateChangeReturn ret = gst_element_set_state(impl->pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE, "Failed to prepare pipeline (PAUSED state)");
        }
        return false;
    }

    //std::cout << "�ȴ�״̬ת����� " << std::endl;
    // �ȴ�״̬ת�����
    GstState state;
    GstState pending;
    GstClockTime timeout = 5 * GST_SECOND;
    ret = gst_element_get_state(impl->pipeline, &state, &pending, timeout);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE, "Failed to get pipeline state");
        }
        return false;
    }

    
   // std::cout << "���ڳ��Բ��� " << std::endl;

    // ���ڳ��Բ���
    ret = gst_element_set_state(impl->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        // ��ȡ���������Ϣ
        GstMessage* msg = gst_bus_poll(gst_element_get_bus(impl->pipeline), GST_MESSAGE_ERROR, 0);
        std::string errorMsg = "Unknown play error";
        if (msg) {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            errorMsg = err ? err->message : "Unknown error";
            if (err) g_error_free(err);
            if (debug) g_free(debug);
            gst_message_unref(msg);
        }

        if (impl->errorCallback) {
            impl->errorCallback(ErrorType::PIPELINE_FAILURE, "Play failed: " + errorMsg);
        }
        return false;
    }

    impl->currentState = State::PLAYING;

   //std::cout << "ȷ���¼�ѭ������ " << std::endl;
    // ȷ���¼�ѭ������
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    return true;
}

bool GStreamerPlayer::pause() {
    if (!impl->pipeline) return false;

    GstStateChangeReturn ret = gst_element_set_state(impl->pipeline, GST_STATE_PAUSED);
    return ret != GST_STATE_CHANGE_FAILURE;
}

bool GStreamerPlayer::stop() {
    if (impl->pipeline) {
        gst_element_set_state(impl->pipeline, GST_STATE_NULL);
        gst_object_unref(impl->pipeline);
        impl->pipeline = nullptr;
    }
    return true;
}

bool GStreamerPlayer::seek(int64_t nanoseconds) {
    if (!impl->pipeline) return false;

    return gst_element_seek_simple(impl->pipeline, GST_FORMAT_TIME,
        static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
        nanoseconds);
}

void GStreamerPlayer::setErrorCallback(ErrorCallback callback) {
    impl->errorCallback = callback;
}

void GStreamerPlayer::setStateChangedCallback(StateChangedCallback callback) {
    impl->stateCallback = callback;
}

void GStreamerPlayer::setEOSCallback(EOSCallback callback) {
    impl->eosCallback = callback;
}

GStreamerPlayer::State GStreamerPlayer::getCurrentState() const {
    std::lock_guard<std::mutex> lock(impl->stateMutex);
    return impl->currentState;
}

int64_t GStreamerPlayer::getDuration() const {
    if (!impl->pipeline) return 0;

    gint64 duration = 0;
    if (gst_element_query_duration(impl->pipeline, GST_FORMAT_TIME, &duration)) {
        return duration;
    }
    return 0;
}

int64_t GStreamerPlayer::getCurrentPosition() const {
    if (!impl->pipeline) return 0;

    gint64 position = 0;
    if (gst_element_query_position(impl->pipeline, GST_FORMAT_TIME, &position)) {
        return position;
    }
    return 0;
}