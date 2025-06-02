#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <filesystem>

class Downloader {
public:
    using DownloadCallback = std::function<void(const std::string& file_name, 
                                              const std::string& file_id, 
                                              bool success, 
                                              const std::string& error)>;

    Downloader(const std::string& url_root);
    ~Downloader();

    void add_task(const std::string& url, 
                 const std::string& file_name, 
                 const std::string& file_id, 
                 const std::string& md5, 
                 DownloadCallback callback);

private:
    struct Task {
        std::string url;
        std::string file_name;
        std::string file_id;
        std::string md5;
        DownloadCallback callback;
    };

    std::string url_root_;
    std::queue<Task> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    bool stop_flag_;

    void worker();
    void process_task(const Task& task);
    size_t get_file_size(const std::string& url);
    bool download_file_multithread(const std::string& url, const std::string& local_path);
    bool verify_md5(const std::string& file_path, const std::string& expected_md5);
};

#endif // DOWNLOADER_H