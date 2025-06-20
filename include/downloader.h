#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <filesystem>
#include "task_repository.h"
#include <memory>
#include <vector>
#include <httplib.h>

class Downloader
{
public:
    using DownloadCallback = std::function<void(const MediaItem &media,
                                                const std::string &local_path,
                                                bool success,
                                                const std::string &error)>;

    // struct Task
    // {
    //     // std::string url;
    //     // std::string file_name;
    //     // std::string file_id;
    //     // std::string md5;
    //     // int type; // 任务类型
    //     std::unique_ptr<MediaItem> media;
    //     DownloadCallback callback;
    // };

    Downloader(const std::string &url_root);
    ~Downloader();

    void setDownloadCallback(DownloadCallback callback);
    void add_task(const MediaItem &media);
    void update_url(const std::string &url);

private:
    std::string url_root_;
    std::string work_dir_;
    DownloadCallback callback_;
    std::queue<MediaItem> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    bool stop_flag_;

    void worker();
    void process_task(const MediaItem &task);

    httplib::Result get_http_client(const std::string &url, int timeout = 30);
    size_t get_file_size(const std::string &url);
    bool download_file_multithread(const std::string &url, const std::string &local_path);
    /// @brief 单线程下载文件
    /// @param url
    /// @param local_path
    /// @return
    bool download_file(const std::string &url, const std::string &local_path);
    size_t dl_req_reply(void *buffer, size_t size, size_t nmemb, void *user_p);
    bool verify_md5(const std::string &file_path, const std::string &expected_md5);
};

#endif // DOWNLOADER_H