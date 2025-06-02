#include "downloader.h"
#include <curl/curl.h>
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

Downloader::Downloader(const std::string &url_root)
    : url_root_(url_root), work_dir_("data/files/"), stop_flag_(false)
{
    std::filesystem::create_directory(work_dir_);
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

// void Downloader::add_task(const std::string &url,
//                           const std::string &file_name,
//                           const std::string &file_id,
//                           const std::string &md5,
//                           DownloadCallback callback)
// {
//     std::lock_guard<std::mutex> lock(queue_mutex_);
//     tasks_.emplace(Task{url, file_name, file_id, md5, callback});
//     queue_cv_.notify_one();
// }

void Downloader::add_task(const Task &task, DownloadCallback callback)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    Task new_task = task;         // 复制task
    new_task.callback = callback; // 设置回调函数
    tasks_.push(std::move(new_task));
    queue_cv_.notify_one();
}

void Downloader::worker()
{
    while (true)
    {
        Task task;
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

void Downloader::process_task(const Task &task)
{
    std::string full_url = (task.url.find("http") == 0) ? task.url : url_root_ + task.url;
    std::string local_path = work_dir_ + task.file_name;
    int type = task.type;

    std::cout << "文件url: " << full_url << " local_path:" << local_path << std::endl;

    if (std::filesystem::exists(local_path))
    {
        if (verify_md5(local_path, task.md5))
        {
            task.callback(task.file_name, task.file_id, true, "");
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
            if (!verify_md5(local_path, task.md5))
            {
                success = false;
                error_msg = "MD5 mismatch";
                std::filesystem::remove(local_path);
            }
        }
        else
        {
            error_msg = "Download failed";
        }
    }

    task.callback(task.file_name, task.file_id, success, error_msg);
}

size_t Downloader::get_file_size(const std::string &url)
{
    CURL *curl = curl_easy_init();
    double filesize = 0.0;
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
        }
        curl_easy_cleanup(curl);
    }
    return static_cast<size_t>(filesize);
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

    for (int i = 0; i < thread_count; ++i)
    {
        size_t start = i * part_size;
        size_t end = (i == thread_count - 1) ? file_size - 1 : (start + part_size - 1);

        temp_files[i] = local_path + ".part" + std::to_string(i);

        threads.emplace_back([=, &temp_files]()
                             {
            CURL* curl = curl_easy_init();
            if (!curl) return;

            FILE* fp = fopen(temp_files[i].c_str(), "wb");
            if (!fp) {
                curl_easy_cleanup(curl);
                return;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            std::ostringstream range;
            range << start << "-" << end;
            curl_easy_setopt(curl, CURLOPT_RANGE, range.str().c_str());

            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

            curl_easy_perform(curl);

            fclose(fp);
            curl_easy_cleanup(curl); });
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    std::ofstream output(local_path, std::ios::binary);
    if (!output.is_open())
        return false;

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

/// @brief 单线程下载文件
/// @param url
/// @param local_path
/// @return
bool Downloader::download_file(const std::string &url, const std::string &local_path)
{
    size_t file_size = get_file_size(url);
    std::cout << "获取到文件大小: " << file_size << std::endl;
    if (file_size == 0)
        return false;

    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    FILE *fp = fopen(local_path.c_str(), "wb");
    if (!fp)
    {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    return true;
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

    return md5_str.str() == expected_md5;
}