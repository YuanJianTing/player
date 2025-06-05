#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>

class Tools
{
public:
    // 获取存储目录
    static std::string get_work_dir();

    // 获取下载文件保存目录
    static std::string get_download_dir();

    // 获取任务保存目录
    static std::string get_repository_dir();
};

#endif // TOOLS_H