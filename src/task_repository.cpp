#include "task_repository.h"
#include <string>
#include <functional>
#include <filesystem>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <cerrno>  // for errno
#include <cstring> // for strerror
#include <memory>
#include <vector>
#include "Tools.h"

TaskRepository::TaskRepository()
{
    work_dir_ = Tools::get_repository_dir();
    std::filesystem::create_directory(work_dir_);
}

TaskRepository::~TaskRepository()
{
}

std::string TaskRepository::saveTask(std::string data)
{
    try
    {
        Json::Value root;
        Json::Reader reader;
        bool parsingSuccessful = reader.parse(data, root);
        if (!parsingSuccessful)
        {
            std::cerr << "数据解析失败： " << data << std::endl;
            return "";
        }
        std::string device = root["device"].asString();
        std::string taskId = root["taskId"].asString();
        int taskCode = root["taskCode"].asInt();
        Json::Value items = root["videoTasks"];
        // 将任务保存到磁盘
        saveTaskId(device, taskId);
        Json::StreamWriterBuilder wbuilder;
        std::string result = Json::writeString(wbuilder, items);
        saveFile(device + ".json", result);
        return device;
    }
    catch (const std::exception &e)
    {
        std::cerr << "save task fail:" << e.what() << '\n';
    }
    return "";
}

std::vector<std::unique_ptr<MediaItem>> TaskRepository::getPlayList(const std::string &device_id)
{
    std::string jsonString = readFile(device_id + ".json");
    if (jsonString.empty())
    {
        return {};
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errors;

    if (!reader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.size(), &root, &errors))
    {
        std::cerr << "JSON解析错误: " << errors << std::endl;
        return {};
    }

    std::vector<std::unique_ptr<MediaItem>> playList;

    for (const auto &item : root)
    {
        auto mediaItem = std::make_unique<MediaItem>();

        // 解析必填字段
        mediaItem->id = item["Id"].asString();
        mediaItem->confirm_url = item["ConfirmURL"].asString();
        mediaItem->download_url = item["DownloadURL"].asString();
        mediaItem->file_name = item["FileName"].asString();
        mediaItem->group = item["Group"].asInt();
        mediaItem->index = item["Index"].asInt();
        mediaItem->left = item["Left"].asInt();
        mediaItem->top = item["Top"].asInt();
        mediaItem->type = item["Type"].asInt();
        mediaItem->width = item["Width"].asInt();
        mediaItem->height = item["Height"].asInt();
        mediaItem->MD5 = item["MD5"].asString();
        mediaItem->size = item["Size"].asInt64();
        mediaItem->device_id = device_id; // 使用传入的参数
        mediaItem->sync_play = item["SyncPlay"].asBool();
        mediaItem->play_time = item["Playtime"].asInt();
        // // 处理可能不存在的字段
        // if (item.isMember("TaskId"))
        // {
        //     // 如果有需要可以添加到结构体中
        // }
        playList.push_back(std::move(mediaItem));
    }
    return playList;
}

std::string TaskRepository::getTaskId(std::string device_id)
{
    return readFile(device_id + ".task");
}

void TaskRepository::saveTaskId(std::string device_id, std::string task_id)
{
    saveFile(device_id + ".task", task_id);
}

std::string TaskRepository::readFile(std::string file_name)
{
    std::string full_path = work_dir_ + file_name;
    // 判断文件是否存在
    if (std::filesystem::exists(full_path))
    {
        std::ifstream file(full_path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("无法打开文件: " + full_path);
        }
        // 高效读取方式（C++17起支持）
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string content;
        content.resize(size);

        if (!file.read(&content[0], size))
        {
            throw std::runtime_error("读取文件内容失败");
        }
        return content;
    }
    return std::string();
}

void TaskRepository::saveFile(std::string file_name, std::string content)
{
    // 检查文件名非空
    if (file_name.empty())
    {
        throw std::invalid_argument("文件名不能为空");
    }

    // 拼接完整路径
    std::string full_path = work_dir_ + file_name;

    // 打开文件（二进制写入模式）
    FILE *fp = fopen(full_path.c_str(), "wb");
    if (!fp)
    {
        throw std::runtime_error("无法打开文件 " + full_path + ": " + strerror(errno));
    }

    try
    {
        // 写入内容
        size_t bytes_written = fwrite(content.data(), sizeof(char), content.size(), fp);
        // 验证写入完整性
        if (bytes_written != content.size())
        {
            throw std::runtime_error("文件写入不完整，预期写入 " +
                                     std::to_string(content.size()) +
                                     " 字节，实际写入 " +
                                     std::to_string(bytes_written) + " 字节");
        }
        // 刷新缓冲区确保数据写入磁盘
        if (fflush(fp) != 0)
        {
            throw std::runtime_error("刷新缓冲区失败: " + std::string(strerror(errno)));
        }
        fclose(fp);
    }
    catch (...)
    {
        // 发生异常时确保文件句柄关闭
        fclose(fp);
        throw; // 重新抛出异常
    }
}