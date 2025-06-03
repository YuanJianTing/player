#ifndef TASK_REPOSITORY_H
#define TASK_REPOSITORY_H

#include <string>
#include <memory>
#include <vector>

struct MediaItem
{
    std::string id;
    std::string confirm_url;
    std::string download_url;
    std::string file_name;
    int group;
    int index;
    int left;
    int top;
    int type; // 0 image 1 video
    int width;
    int height;
    std::string MD5;
    int64_t size;
    std::string device_id;
    bool sync_play;
    int play_time;
};

class TaskRepository
{
public:
    struct PlayList
    {
        std::string device;
        std::string taskId;
        int taskCode;
        std::vector<MediaItem> list;
    };

    TaskRepository();
    ~TaskRepository();

    std::string saveTask(std::string data);
    std::vector<std::unique_ptr<MediaItem>> getPlayList(const std::string &device_id);
    std::string getTaskId(std::string device_id);

private:
    void saveTaskId(std::string device_id, std::string task_id);
    void saveFile(std::string file_name, std::string content);
    std::string readFile(std::string file_name);
    std::string work_dir_;
};

#endif // TASK_REPOSITORY_H
