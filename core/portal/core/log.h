//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <map>
#include <thread>

#include <spdlog/mdc.h>
#include <spdlog/spdlog.h>

#include "custom_logger_formatters.h"

#define PORTAL_ASSERT_MESSAGE_BOX (!PORTAL_DIST && PORTAL_PLATFORM_WINDOWS)

namespace portal
{
class ScopedLogContext
{
public:
    template <typename... Args>
    explicit ScopedLogContext(Args&&... args)
    {
        static_assert(sizeof...(args) % 2 == 0, "Must provide even number of arguments (key-value pairs)");
        add_pairs(std::forward<Args>(args)...);
    }

    ~ScopedLogContext()
    {
        for (const auto& key : keys)
            spdlog::mdc::remove(key);
    }

private:
    void add_pairs() {}

    template <typename K, typename V, typename... Rest>
    void add_pairs(K&& key, V&& value, Rest&&... rest)
    {
        const std::string k = to_string(std::forward<K>(key));
        const std::string v = to_string(std::forward<V>(value));
        spdlog::mdc::put(k, v);
        keys.push_back(k);
        add_pairs(std::forward<Rest>(rest)...);
    }

    template <typename T>
    std::string to_string(T&& val)
    {
        if constexpr (std::is_convertible_v<T, std::string>)
            return std::string(std::forward<T>(val));
        else
        {
            std::ostringstream oss;
            oss << val;
            return oss.str();
        }
    }

    std::vector<std::string> keys;
};


class Log
{
public:
    enum class LogLevel : uint8_t
    {
        Trace = spdlog::level::trace,
        Debug = spdlog::level::debug,
        Info  = spdlog::level::info,
        Warn  = spdlog::level::warn,
        Error = spdlog::level::err,
        Fatal = spdlog::level::critical,
    };

    struct Settings
    {
        LogLevel default_log_level = LogLevel::Trace;
        std::string default_logger_name = "default";
    };

public:
    static void init();
    static void init(const Settings& settings);
    static void shutdown();

    static std::shared_ptr<spdlog::logger> get_logger(const std::string& tag_name);

    static void set_default_log_level(LogLevel level, bool apply_to_all = true);

    static bool has_tag(const std::string& tag_name);
    static void set_tag_level(const std::string& tag_name, LogLevel level);
    static void enable_tag(const std::string& tag_name, bool enable = true);

    template <typename... Args>
    static ScopedLogContext with_context(Args&&... args)
    {
        return ScopedLogContext(std::forward<Args>(args)...);
    }


    template <typename... Args>
    static void print_message_tag(
        spdlog::source_loc loc,
        LogLevel level,
        std::string_view tag,
        spdlog::format_string_t<Args...> format,
        Args&&... args
        );

    static void print_message_tag(
        const spdlog::source_loc& loc,
        LogLevel level,
        std::string_view tag,
        std::string_view message
        );

    template <typename... Args>
    static void print_message(
        const std::shared_ptr<spdlog::logger>& logger,
        const spdlog::source_loc& loc,
        LogLevel level,
        spdlog::format_string_t<Args...> format,
        Args&&... args
        );

    static void print_message(
        const std::shared_ptr<spdlog::logger>& logger,
        const spdlog::source_loc& loc,
        LogLevel level,
        std::string_view message
        );

    template <typename... Args>
    static bool print_assert_message(
        std::string_view file,
        int line,
        std::string_view function,
        spdlog::format_string_t<Args...> format,
        Args&&... args
        );

    static bool print_assert_message(std::string_view file, int line, std::string_view function, std::string_view message);
private:
    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>& get_loggers();
};
} // namespace portal

#define SOURCE_LOC spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tagged logs (prefer these!)                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TRACE_TAG(tag, ...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Trace, tag, __VA_ARGS__)
#define LOG_DEBUG_TAG(tag, ...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Debug, tag, __VA_ARGS__)
#define LOG_INFO_TAG(tag, ...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Info, tag, __VA_ARGS__)
#define LOG_WARN_TAG(tag, ...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Warn, tag, __VA_ARGS__)
#define LOG_ERROR_TAG(tag, ...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Error, tag, __VA_ARGS__)
#define LOG_FATAL_TAG(tag, ...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Fatal, tag, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Default logs                                                                                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TRACE(...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Trace, "default", __VA_ARGS__)
#define LOG_DEBUG(...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Debug, "default", __VA_ARGS__)
#define LOG_INFO(...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Info, "default", __VA_ARGS__)
#define LOG_WARN(...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Warn, "default", __VA_ARGS__)
#define LOG_ERROR(...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Error, "default", __VA_ARGS__)
#define LOG_FATAL(...) ::portal::Log::print_message_tag(SOURCE_LOC, ::portal::Log::LogLevel::Fatal, "default", __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logger logs (expects a logger named "logger" to be available in the context                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOGGER_TRACE(...) ::portal::Log::print_message(logger, SOURCE_LOC, ::portal::Log::LogLevel::Trace, __VA_ARGS__)
#define LOGGER_DEBUG(...) ::portal::Log::print_message(logger, SOURCE_LOC, ::portal::Log::LogLevel::Debug, __VA_ARGS__)
#define LOGGER_INFO(...) ::portal::Log::print_message(logger, SOURCE_LOC, ::portal::Log::LogLevel::Info, __VA_ARGS__)
#define LOGGER_WARN(...) ::portal::Log::print_message(logger, SOURCE_LOC, ::portal::Log::LogLevel::Warn, __VA_ARGS__)
#define LOGGER_ERROR(...) ::portal::Log::print_message(logger, SOURCE_LOC, ::portal::Log::LogLevel::Error, __VA_ARGS__)
#define LOGGER_FATAL(...) ::portal::Log::print_message(logger, SOURCE_LOC, ::portal::Log::LogLevel::Fatal, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace portal
{
template <typename... Args>
void Log::print_message_tag(
    const spdlog::source_loc loc,
    const LogLevel level,
    const std::string_view tag,
    spdlog::format_string_t<Args...> format,
    Args&&... args
    )
{
#if defined(PORTAL_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4459)
#endif
    if (const auto& logger = get_logger(tag.data()))
    {
        logger->log(loc, static_cast<spdlog::level::level_enum>(level), format, std::forward<Args>(args)...);
    }
#if defined(PORTAL_COMPILER_MSVC)
#pragma warning(pop)
#endif
}


template <typename... Args>
void Log::print_message(
    const std::shared_ptr<spdlog::logger>& logger,
    const spdlog::source_loc& loc,
    LogLevel level,
    spdlog::format_string_t<Args...> format,
    Args&&... args
    )
{
    logger->log(loc, static_cast<spdlog::level::level_enum>(level), format, std::forward<Args>(args)...);
}


template <typename... Args>
bool Log::print_assert_message(
    const std::string_view file,
    const int line,
    const std::string_view function,
    spdlog::format_string_t<Args...> format,
    Args&&... args
    )
{
    const std::string formatted = std::format(format, std::forward<Args>(args)...);
    return print_assert_message(file, line, function, formatted);
}
} // namespace portal
