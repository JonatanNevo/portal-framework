//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <map>

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
    enum class Type : uint8_t
    {
        Core   = 0,
        Client = 1,
    };

    enum class Level : uint8_t
    {
        Trace = 0,
        Debug = 1,
        Info  = 2,
        Warn  = 3,
        Error = 4,
        Fatal = 5,
    };

    struct TagDetails
    {
        bool enabled = true;
        Level level_filter = Level::Trace;
    };

public:
    static void init();
    static void shutdown();

    inline static std::shared_ptr<spdlog::logger>& get_core_logger() { return core_logger; }
    inline static std::shared_ptr<spdlog::logger>& get_client_logger() { return client_logger; }

    static bool has_tag(const std::string& tag) { return enabled_tags.find(tag) != enabled_tags.end(); }
    static std::map<std::string, TagDetails>& get_enabled_tags() { return enabled_tags; }

    template <typename... Args>
    static void print_message_tag(Log::Type type, Log::Level level, std::string_view tag, std::format_string<Args...> format, Args&&... args);

    static void print_message_tag(Log::Type type, Log::Level level, std::string_view tag, std::string_view message);

    template <typename... Args>
    static void print_assert_message(Log::Type type, std::string_view prefix, std::format_string<Args...> format, Args&&... args);

    static void print_assert_message(Log::Type type, std::string_view prefix, std::string_view message);

    static void print_assert_message(Log::Type type, std::string_view prefix);

public:
    static const char* level_to_string(Level level)
    {
        switch (level)
        {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        case Level::Fatal:
            return "FATAL";
        }
        return "";
    }

    static Level level_from_string(std::string_view string)
    {
        if (string == "TRACE")
            return Level::Trace;
        if (string == "DEBUG")
            return Level::Debug;
        if (string == "INFO")
            return Level::Info;
        if (string == "WARN")
            return Level::Warn;
        if (string == "ERROR")
            return Level::Error;
        if (string == "FATAL")
            return Level::Fatal;
        return Level::Trace;
    }

private:
    static std::shared_ptr<spdlog::logger> core_logger;
    static std::shared_ptr<spdlog::logger> client_logger;

    inline static std::map<std::string, TagDetails> enabled_tags;
};

} // namespace portal


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tagged logs (prefer these!)                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core logging
#define LOG_CORE_TRACE_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Trace, tag, __VA_ARGS__)
#define LOG_CORE_DEBUG_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Debug, tag, __VA_ARGS__)
#define LOG_CORE_INFO_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Info, tag, __VA_ARGS__)
#define LOG_CORE_WARN_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Warn, tag, __VA_ARGS__)
#define LOG_CORE_ERROR_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Error, tag, __VA_ARGS__)
#define LOG_CORE_FATAL_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Fatal, tag, __VA_ARGS__)

// Client logging
#define LOG_TRACE_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Trace, tag, __VA_ARGS__)
#define LOG_DEBUG_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Debug, tag, __VA_ARGS__)
#define LOG_INFO_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Info, tag, __VA_ARGS__)
#define LOG_WARN_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Warn, tag, __VA_ARGS__)
#define LOG_ERROR_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Error, tag, __VA_ARGS__)
#define LOG_FATAL_TAG(tag, ...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Fatal, tag, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core logging
#define LOG_CORE_TRACE(...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Trace, "CORE", __VA_ARGS__)
#define LOG_CORE_DEBUG(...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Debug, "CORE", __VA_ARGS__)
#define LOG_CORE_INFO(...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Info, "CORE", __VA_ARGS__)
#define LOG_CORE_WARN(...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Warn, "CORE", __VA_ARGS__)
#define LOG_CORE_ERROR(...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Error, "CORE", __VA_ARGS__)
#define LOG_CORE_FATAL(...) ::portal::Log::print_message_tag(::portal::Log::Type::Core, ::portal::Log::Level::Fatal, "CORE", __VA_ARGS__)

// Client logging
#define LOG_TRACE(...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Trace, "CLIENT", __VA_ARGS__)
#define LOG_DEBUG(...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Debug, "CLIENT", __VA_ARGS__)
#define LOG_INFO(...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Info, "CLIENT", __VA_ARGS__)
#define LOG_WARN(...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Warn, "CLIENT", __VA_ARGS__)
#define LOG_ERROR(...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Error, "CLIENT", __VA_ARGS__)
#define LOG_FATAL(...) ::portal::Log::print_message_tag(::portal::Log::Type::Client, ::portal::Log::Level::Fatal, "CLIENT", __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace portal
{
template <typename... Args>
void Log::print_message_tag(
    const Log::Type type,
    const Log::Level level,
    const std::string_view tag,
    std::format_string<Args...> format,
    Args&&... args
    )
{
    const std::string formatted = std::format(format, std::forward<Args>(args)...);
    print_message_tag(type, level, tag, formatted);
}

inline void Log::print_message_tag(const Log::Type type, const Log::Level level, std::string_view tag, std::string_view message)
{
    const auto [enabled, level_filter] = enabled_tags[std::string(tag)];
    if (enabled && level_filter <= level)
    {
        const auto logger = (type == Type::Core) ? core_logger : client_logger;
        switch (level)
        {
        case Level::Trace:
            logger->trace("[{}] {}", tag, message);
            break;
        case Level::Debug:
            logger->debug("[{}] {}", tag, message);
            break;
        case Level::Info:
            logger->info("[{}] {}", tag, message);
            break;
        case Level::Warn:
            logger->warn("[{}] {}", tag, message);
            break;
        case Level::Error:
            logger->error("[{}] {}", tag, message);
            break;
        case Level::Fatal:
            logger->critical("[{}] {}", tag, message);
            break;
        }
    }
}


template <typename... Args>
void Log::print_assert_message(const Log::Type type, std::string_view prefix, std::format_string<Args...> format, Args&&... args)
{

    const std::string formatted = std::format(format, std::forward<Args>(args)...);
    print_assert_message(type, prefix, formatted);
}

inline void Log::print_assert_message(const Log::Type type, std::string_view prefix, std::string_view message)
{
    const auto logger = (type == Type::Core) ? core_logger : client_logger;
    logger->error("{0}: {1}", prefix, message);

#if PORTAL_ASSERT_MESSAGE_BOX
    MessageBoxA(nullptr, message.data(), "Walnut Assertion", MB_OK | MB_ICONERROR);
#endif
}

inline void Log::print_assert_message(const Log::Type type, std::string_view prefix)
{
    print_assert_message(type, prefix, "No message :(");
}


} // namespace portal
