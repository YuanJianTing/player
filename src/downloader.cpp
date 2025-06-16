#include "downloader.h"
#include <httplib.h>
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <filesystem>
#include "Tools.h"

Downloader::Downloader(const std::string &url_root)
    : url_root_(url_root), stop_flag_(false)
{
    work_dir_ = Tools::get_download_dir();
    std::filesystem::create_directory(work_dir_);
    // 解析URL获取主机和端口
    size_t protocol_pos = url_root.find("://");
    if (protocol_pos == std::string::npos)
    {
        throw std::runtime_error("Invalid URL format");
    }

    std::string protocol = url_root.substr(0, protocol_pos);
    std::string host_port_path = url_root.substr(protocol_pos + 3);

    size_t slash_pos = host_port_path.find('/');
    std::string host_port = host_port_path.substr(0, slash_pos);

    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos)
    {
        std::string host_ = host_port.substr(0, colon_pos);
        int port_ = std::stoi(host_port.substr(colon_pos + 1));
    }
    else
    {
        std::string host_ = host_port;
        int port_ = (protocol == "https") ? 443 : 80;
    }

    is_https_ = (protocol == "https");
    worker_thread_ = std::thread(&Downloader::worker, this);
}

Downloader::~Downloader()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_flag_ = true;
    }
    queue_cv_.notify_all();
    if (worker_thread_.joinable())
    {
        worker_thread_.join();
    }
}

void Downloader::setDownloadCallback(DownloadCallback callback)
{
    callback_ = callback;
}

void Downloader::add_task(const MediaItem &task)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    tasks_.push(task);
    queue_cv_.notify_one();
}

void Downloader::worker()
{
    while (true)
    {
        MediaItem task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [&]
                           { return stop_flag_ || !tasks_.empty(); });

            if (stop_flag_ && tasks_.empty())
                return;

            task = tasks_.front();
            tasks_.pop();
        }
        process_task(task);
    }
}

void Downloader::process_task(const MediaItem &task)
{
    std::string download_url = task.download_url;
    std::string full_url = (download_url.find("http") == 0) ? download_url : url_root_ + download_url;

    std::filesystem::path p(task.file_name);
    std::string extension = p.extension().string();
    std::string local_path = work_dir_ + task.MD5 + extension;
    int type = task.type;

    if (std::filesystem::exists(local_path))
    {
        if (verify_md5(local_path, task.MD5))
        {
            callback_(task, local_path, true, "");
            return;
        }
        else
        {
            std::filesystem::remove(local_path);
        }
    }

    const int max_retries = 5;
    int attempt = 0;
    bool success = false;
    std::string error_msg;

    while (attempt < max_retries && !success)
    {
        attempt++;
        if (type == 0)
        {
            // 主题图片，不支持多线程下载
            success = download_file(full_url, local_path);
        }
        else
        {
            success = download_file_multithread(full_url, local_path);
        }

        if (success)
        {
            if (!verify_md5(local_path, task.MD5))
            {
                std::cout << "MD5验证不通过。\n"
                          << local_path << std::endl;
                success = false;
                error_msg = "MD5 mismatch";
                std::filesystem::remove(local_path);
                break;
            }
        }
        else
        {
            std::cout << "单次下载文件失败" << std::endl;
            error_msg = "Download failed";
        }
    }

    callback_(task, local_path, success, error_msg);
}

size_t Downloader::get_file_size(const std::string &url)
{
    try
    {
        size_t protocol_pos = url.find("://");
        if (protocol_pos == std::string::npos)
        {
            return 0;
        }

        std::string protocol = url.substr(0, protocol_pos);
        std::string host_port_path = url.substr(protocol_pos + 3);

        size_t slash_pos = host_port_path.find('/');
        std::string path = host_port_path.substr(slash_pos);

        httplib::Client client(url_root_);
        client.set_connection_timeout(10);

        if (is_https_)
        {
            client.enable_server_certificate_verification(false);
        }

        auto res = client.Head(path.c_str());
        if (res && res->has_header("Content-Length"))
        {
            return std::stoul(res->get_header_value("Content-Length"));
        }
    }
    catch (...)
    {
        return 0;
    }
    return 0;
}

bool Downloader::download_file_multithread(const std::string &url, const std::string &local_path)
{
    size_t file_size = get_file_size(url);
    std::cout << "获取到文件大小: " << file_size << std::endl;
    if (file_size == 0)
        return false;

    const int thread_count = 4;
    size_t part_size = file_size / thread_count;

    std::vector<std::thread> threads;
    std::vector<std::string> temp_files(thread_count);
    std::vector<bool> download_results(thread_count, false);

    size_t protocol_pos = url.find("://");
    if (protocol_pos == std::string::npos)
    {
        return 0;
    }

    std::string protocol = url.substr(0, protocol_pos);
    std::string host_port_path = url.substr(protocol_pos + 3);

    size_t slash_pos = host_port_path.find('/');
    std::string path = host_port_path.substr(slash_pos);

    for (int i = 0; i < thread_count; ++i)
    {
        size_t start = i * part_size;
        size_t end = (i == thread_count - 1) ? file_size - 1 : (start + part_size - 1);

        temp_files[i] = local_path + ".part" + std::to_string(i);

        threads.emplace_back([=, &download_results]()
                             {
            try {
                httplib::Client client(url_root_);
                client.set_connection_timeout(10);
                client.set_read_timeout(30);
                
                if (protocol == "https") {
                    client.enable_server_certificate_verification(false);
                }

                std::string range = "bytes=" + std::to_string(start) + "-" + std::to_string(end);
                auto res = client.Get(path.c_str(), {
                    {"Range", range}
                });

                if (res && res->status == 206) {
                    std::ofstream out(temp_files[i], std::ios::binary);
                    if (out) {
                        out << res->body;
                        out.close();
                        download_results[i] = true;
                    }
                }
            } catch (...) {
                download_results[i] = false;
            } });
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    // 检查所有部分是否下载成功
    for (bool result : download_results)
    {
        if (!result)
        {
            // 清理临时文件
            for (const auto &temp_file : temp_files)
            {
                std::filesystem::remove(temp_file);
            }
            return false;
        }
    }

    // 合并文件
    std::ofstream output(local_path, std::ios::binary);
    if (!output.is_open())
    {
        return false;
    }

    for (int i = 0; i < thread_count; ++i)
    {
        std::ifstream part(temp_files[i], std::ios::binary);
        output << part.rdbuf();
        part.close();
        std::filesystem::remove(temp_files[i]);
    }

    output.close();
    return true;
}

bool Downloader::download_file(const std::string &url, const std::string &local_path)
{
    try
    {
        size_t protocol_pos = url.find("://");
        if (protocol_pos == std::string::npos)
        {
            return false;
        }

        std::string protocol = url.substr(0, protocol_pos);
        std::string host_port_path = url.substr(protocol_pos + 3);

        size_t slash_pos = host_port_path.find('/');
        std::string path = host_port_path.substr(slash_pos);

        httplib::Client client(url_root_);
        client.set_connection_timeout(10);
        client.set_read_timeout(30);

        if (is_https_)
        {
            client.enable_server_certificate_verification(false);
        }

        auto res = client.Get(path.c_str());
        if (res && res->status == 200)
        {
            std::ofstream out(local_path, std::ios::binary);
            if (out)
            {
                out << res->body;
                return true;
            }
        }
    }
    catch (...)
    {
        return false;
    }
    return false;
}

bool Downloader::verify_md5(const std::string &file_path, const std::string &expected_md5)
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file)
        return false;

    EVP_MD_CTX *md5_ctx = EVP_MD_CTX_new();
    if (!md5_ctx)
        return false;

    if (EVP_DigestInit_ex(md5_ctx, EVP_md5(), NULL) != 1)
    {
        EVP_MD_CTX_free(md5_ctx);
        return false;
    }

    char buf[4096];
    while (file.read(buf, sizeof(buf)))
    {
        if (EVP_DigestUpdate(md5_ctx, buf, file.gcount()) != 1)
        {
            EVP_MD_CTX_free(md5_ctx);
            return false;
        }
    }
    if (file.gcount() > 0)
    {
        if (EVP_DigestUpdate(md5_ctx, buf, file.gcount()) != 1)
        {
            EVP_MD_CTX_free(md5_ctx);
            return false;
        }
    }

    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len;
    if (EVP_DigestFinal_ex(md5_ctx, result, &result_len) != 1)
    {
        EVP_MD_CTX_free(md5_ctx);
        return false;
    }

    EVP_MD_CTX_free(md5_ctx);

    std::ostringstream md5_str;
    for (unsigned int i = 0; i < result_len; ++i)
    {
        md5_str << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    }

    std::cout << "计算的MD5: " << md5_str.str() << std::endl;
    std::cout << "期望的MD5: " << expected_md5 << std::endl;

    return md5_str.str() == expected_md5;
}

void Downloader::update_url(const std::string &url)
{
    url_root_ = url;
    // 解析URL获取主机和端口
    size_t protocol_pos = url.find("://");
    if (protocol_pos == std::string::npos)
    {
        throw std::runtime_error("Invalid URL format");
    }

    std::string protocol = url.substr(0, protocol_pos);
    std::string host_port_path = url.substr(protocol_pos + 3);

    size_t slash_pos = host_port_path.find('/');
    std::string host_port = host_port_path.substr(0, slash_pos);

    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos)
    {
        std::string host_ = host_port.substr(0, colon_pos);
        int port_ = std::stoi(host_port.substr(colon_pos + 1));
    }
    else
    {
        std::string host_ = host_port;
        int port_ = (protocol == "https") ? 443 : 80;
    }

    is_https_ = (protocol == "https");
}