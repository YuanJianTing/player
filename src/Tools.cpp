#include "Tools.h"
#include <string>
#include <vector>

std::string Tools::get_work_dir()
{
    const char *env_path = std::getenv("EPLAYER_DIR");
    return env_path ? env_path : "/data/";
}

std::string Tools::get_download_dir()
{
    const std::string root = get_work_dir();
    return root + "download/";
}

std::string Tools::get_repository_dir()
{
    const std::string root = get_work_dir();
    return root + "task/";
}