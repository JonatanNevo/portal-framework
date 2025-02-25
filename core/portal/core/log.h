//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include <map>
#include <thread>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "custom_logger_formatters.h"

#define PORTAL_ASSERT_MESSAGE_BOX (!PORTAL_DIST && PORTAL_PLATFORM_WINDOWS)

#if defined(PORTAL_ASSERT_MESSAGE_BOX) && !defined(PORTAL_DIST)
#ifdef PORTAL_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif

namespace portal
{
class Log
{
public:
    enum class LoggerType : uint8_t
    {
        Core   = 0,
        Client = 1,
    };

    enum class LogLevel : uint8_t
    {
        Trace = spdlog::level::trace,
        Debug = spdlog::level::debug,
        Info  = spdlog::level::info,
        Warn  = spdlog::level::warn,
        Error = spdlog::level::err,
        Fatal = spdlog::level::critical,
    };

    struct TagDetails
    {
        bool enabled = true;
        LogLevel level_filter = LogLevel::Trace;
    };

public:
    static void init();
    static void shutdown();

    static bool is_main_thread()
    {
        return std::this_thread::get_id() == main_thread_id;
    }

    static std::shared_ptr<spdlog::logger>& get_core_logger() { return core_logger; }
    static std::shared_ptr<spdlog::logger>& get_client_logger() { return client_logger; }

    static bool has_tag(const std::string& tag) { return enabled_tags.contains(tag); }
    static std::map<std::string, TagDetails>& get_enabled_tags() { return enabled_tags; }

    template <typename... Args>
    static void print_message_tag(LoggerType type, LogLevel level, std::string_view tag, std::format_string<Args...> format, Args&&... args);

    static void print_message_tag(LoggerType type, LogLevel level, std::string_view tag, std::string_view message);

    template <typename... Args>
    static bool print_assert_message(
        LoggerType type,
        std::string_view file,
        int line,
        std::string_view function,
        std::format_string<Args...> format,
        Args&&... args
    );

    static bool print_assert_message(LoggerType type, std::string_view file, int line, std::string_view function, std::string_view message);

public:
    static const char* level_to_string(const LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
        }
        return "";
    }

    static LogLevel level_from_string(const std::string_view string)
    {
        if (string == "TRACE")
            return LogLevel::Trace;
        if (string == "DEBUG")
            return LogLevel::Debug;
        if (string == "INFO")
            return LogLevel::Info;
        if (string == "WARN")
            return LogLevel::Warn;
        if (string == "ERROR")
            return LogLevel::Error;
        if (string == "FATAL")
            return LogLevel::Fatal;
        return LogLevel::Trace;
    }

private:
    static std::thread::id main_thread_id;
    static std::shared_ptr<spdlog::logger> core_logger;
    static std::shared_ptr<spdlog::logger> client_logger;

    inline static std::map<std::string, TagDetails> enabled_tags;
};
} // namespace portal


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tagged logs (prefer these!)                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core logging
#define LOG_CORE_TRACE_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Trace, tag, __VA_ARGS__)
#define LOG_CORE_DEBUG_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Debug, tag, __VA_ARGS__)
#define LOG_CORE_INFO_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Info, tag, __VA_ARGS__)
#define LOG_CORE_WARN_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Warn, tag, __VA_ARGS__)
#define LOG_CORE_ERROR_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Error, tag, __VA_ARGS__)
#define LOG_CORE_FATAL_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Fatal, tag, __VA_ARGS__)

// Client logging
#define LOG_TRACE_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Trace, tag, __VA_ARGS__)
#define LOG_DEBUG_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Debug, tag, __VA_ARGS__)
#define LOG_INFO_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Info, tag, __VA_ARGS__)
#define LOG_WARN_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Warn, tag, __VA_ARGS__)
#define LOG_ERROR_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Error, tag, __VA_ARGS__)
#define LOG_FATAL_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Fatal, tag, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core logging
#define LOG_CORE_TRACE(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Trace, "CORE", __VA_ARGS__)
#define LOG_CORE_DEBUG(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Debug, "CORE", __VA_ARGS__)
#define LOG_CORE_INFO(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Info, "CORE", __VA_ARGS__)
#define LOG_CORE_WARN(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Warn, "CORE", __VA_ARGS__)
#define LOG_CORE_ERROR(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Error, "CORE", __VA_ARGS__)
#define LOG_CORE_FATAL(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Core, ::portal::Log::LogLevel::Fatal, "CORE", __VA_ARGS__)

// Client logging
#define LOG_TRACE(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Trace, "CLIENT", __VA_ARGS__)
#define LOG_DEBUG(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Debug, "CLIENT", __VA_ARGS__)
#define LOG_INFO(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Info, "CLIENT", __VA_ARGS__)
#define LOG_WARN(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Warn, "CLIENT", __VA_ARGS__)
#define LOG_ERROR(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Error, "CLIENT", __VA_ARGS__)
#define LOG_FATAL(...) ::portal::Log::print_message_tag(::portal::Log::LoggerType::Client, ::portal::Log::LogLevel::Fatal, "CLIENT", __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace portal
{
template <typename... Args>
void Log::print_message_tag(
    const LoggerType type,
    const LogLevel level,
    const std::string_view tag,
    std::format_string<Args...> format,
    Args&&... args
)
{
    const std::string formatted = std::format(format, std::forward<Args>(args)...);
    print_message_tag(type, level, tag, formatted);
}

inline void Log::print_message_tag(const LoggerType type, const LogLevel level, std::string_view tag, std::string_view message)
{
    const auto [enabled, level_filter] = enabled_tags[std::string(tag)];
    if (enabled && level_filter <= level)
    {
        const auto logger = (type == LoggerType::Core) ? core_logger : client_logger;
        logger->log(static_cast<spdlog::level::level_enum>(level), "[{}] {}", tag, message);
    }
}


template <typename... Args>
bool Log::print_assert_message(
    const LoggerType type,
    const std::string_view file,
    const int line,
    const std::string_view function,
    std::format_string<Args...> format,
    Args&&... args
)
{
    const std::string formatted = std::format(format, std::forward<Args>(args)...);
    return print_assert_message(type, file, line, function, formatted);
}
} // namespace portal
