// logger.h
#pragma once
#include <cstdio>
#include <ctime>
#include <cstdarg>

enum LogLevel
{
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class ConsoleLogger
{
public:
    static void log(LogLevel level, const char *tag, const char *fmt, ...)
    {
        // 获取可变参数
        va_list args;
        va_start(args, fmt);

        // 日志级别前缀
        const char *levelStr = "";
        switch (level)
        {
        case VERBOSE:
            levelStr = "V";
            break;
        case DEBUG:
            levelStr = "D";
            break;
        case INFO:
            levelStr = "I";
            break;
        case WARN:
            levelStr = "W";
            break;
        case ERROR:
            levelStr = "E";
            break;
        default:
            levelStr = "?";
            break;
        }

        // 生成时间戳 (格式: MM-DD HH:MM:SS)
        time_t now = time(nullptr);
        struct tm *tm = localtime(&now);

        // 输出日志头 [时间 级别/标签]
        fprintf(stderr, "%02d-%02d %02d:%02d:%02d %s/%s: ",
                tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                levelStr, tag);

        // 输出用户日志内容
        vfprintf(stderr, fmt, args);
        fputc('\n', stderr);

        va_end(args);
    }

    // 新增：直接支持std::string的重载
    static void log(LogLevel level, const char *tag, const std::string &msg)
    {
        log(level, tag, "%s", msg.c_str()); // 转换为C字符串处理
    }
};

#define LOGV(tag, ...) ConsoleLogger::log(VERBOSE, tag, __VA_ARGS__)
#define LOGD(tag, ...) ConsoleLogger::log(DEBUG, tag, __VA_ARGS__)
#define LOGI(tag, ...) ConsoleLogger::log(INFO, tag, __VA_ARGS__)
#define LOGW(tag, ...) ConsoleLogger::log(WARN, tag, __VA_ARGS__)
#define LOGE(tag, ...) ConsoleLogger::log(ERROR, tag, __VA_ARGS__)