#include "Tools.h"
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <iostream>
#include <array>
#include <memory>
#include <regex>

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

std::string Tools::get_device_ip()
{
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];
    std::string ipAddress;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return "";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        family = ifa->ifa_addr->sa_family;
        // 检查是否是无线网卡 (通常以wlan或wlp开头)
        if (strstr(ifa->ifa_name, "wlan") || strstr(ifa->ifa_name, "wlp"))
        {
            if (family == AF_INET)
            { // IPv4
                getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
                ipAddress = host;

                std::cout << "设备: " << family << "   " << ifa->ifa_name << std::endl;

                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ipAddress;
}

ImageData Tools::create_image(const uint32_t color, const int img_width, const int img_height)
{
    ImageData result;
    result.width = img_width;
    result.height = img_height;
    result.channels = 4; // 强制使用RGBA
    result.pixels.resize(img_width * img_height * 4);

    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;

    for (int i = 0; i < img_width * img_height; ++i)
    {
        result.pixels[i * 4 + 0] = r;    // R
        result.pixels[i * 4 + 1] = g;    // G
        result.pixels[i * 4 + 2] = b;    // B
        result.pixels[i * 4 + 3] = 0xFF; // A
    }

    return result;
}

std::string Tools::get_current_wifi_ssid()
{
    std::string interface = "wlp2s0";

    std::string command = "iwconfig " + interface + " | grep 'ESSID:'";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    // 提取SSID
    std::regex ssid_regex("ESSID:\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(result, match, ssid_regex))
    {
        return match[1].str();
    }
    return "";
}

std::string Tools::get_version()
{
    return "v3.0.1";
}

std::string Tools::get_firmware()
{
    return "Debian 6.1.140-1 (2025-05-22)";
}