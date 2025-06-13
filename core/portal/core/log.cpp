//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>
#include <map>
#include <ranges>
#include <utility>

#include "portal/core/common.h"

#define PORTAL_HAS_CONSOLE !PORTAL_DIST


#if defined(PORTAL_ASSERT_MESSAGE_BOX) && !defined(PORTAL_DIST)
#ifdef PORTAL_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif


namespace portal
{

std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Log::loggers;
std::vector<spdlog::sink_ptr> default_sinks{};
Log::Settings g_settings;

// Format:  date [name] colored{[level]} message (file:line function #thread_id) extra
constexpr auto default_pattern = "[%Y-%m-%d %H:%M:%S.%f] [%t] [%n] %^[%l]%$ [%s:%#] %v %&";

void Log::init()
{
    const Settings settings;
    init(settings);
}

void Log::init(const Settings& settings)
{
    g_settings = settings;

    const std::string log_directory = "logs";
    if (!std::filesystem::exists(log_directory))
        std::filesystem::create_directory(log_directory);

    default_sinks = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/portal.log", true),
#if PORTAL_HAS_CONSOLE
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
    };

    for (const auto& sink : default_sinks)
    {
        sink->set_pattern(default_pattern);
    }

    const auto default_logger = std::make_shared<spdlog::logger>(settings.default_logger_name, begin(default_sinks), end(default_sinks));
    default_logger->set_level(static_cast<spdlog::level::level_enum>(settings.default_log_level));

    spdlog::set_default_logger(default_logger);
    loggers["default"] = default_logger;
}

void Log::shutdown()
{
    for (auto& logger : loggers | std::views::values)
    {
        logger.reset();
    }
    spdlog::drop_all();
}

std::shared_ptr<spdlog::logger>& Log::get_logger(const std::string& tag_name)
{
    if (loggers.contains(tag_name))
        return loggers[tag_name];

    // Create a new logger if it doesn't exist
    const auto logger = std::make_shared<spdlog::logger>(tag_name, begin(default_sinks), end(default_sinks));
    logger->set_level(static_cast<spdlog::level::level_enum>(g_settings.default_log_level));
    loggers[tag_name] = logger;
    return loggers[tag_name];
}

void Log::set_default_log_level(LogLevel level, const bool apply_to_all)
{
    g_settings.default_log_level = level;

    // Set the default logger level
    spdlog::default_logger_raw()->set_level(static_cast<spdlog::level::level_enum>(level));

    // Apply to all existing loggers if requested
    if (apply_to_all)
    {
        for (const auto& logger : loggers | std::views::values)
        {
            logger->set_level(static_cast<spdlog::level::level_enum>(level));
        }
    }
}

bool Log::has_tag(const std::string& tag_name)
{
    return loggers.contains(tag_name);
}

void Log::set_tag_level(const std::string& tag_name, LogLevel level)
{
    const auto& logger = get_logger(tag_name);
    logger->set_level(static_cast<spdlog::level::level_enum>(level));
}

void Log::enable_tag(const std::string& tag_name, const bool enable)
{
    // TODO: Save old state?
    const auto& logger = get_logger(tag_name);
    if (enable)
    {
        logger->set_level(static_cast<spdlog::level::level_enum>(g_settings.default_log_level));
    }
    else
    {
        logger->set_level(spdlog::level::off);
    }
}

PORTAL_FORCE_INLINE void Log::print_message_tag(
    const spdlog::source_loc& loc,
    LogLevel level,
    const std::string_view tag,
    const std::string_view message
    )
{
    if (const auto& logger = get_logger(tag.data()))
    {
        logger->log(loc, static_cast<spdlog::level::level_enum>(level), message);
    }
}

PORTAL_FORCE_INLINE void Log::print_message(
    const std::shared_ptr<spdlog::logger>& logger,
    const spdlog::source_loc& loc,
    LogLevel level,
    const std::string_view message
    )
{
    logger->log(loc, static_cast<spdlog::level::level_enum>(level), message);
}

bool Log::print_assert_message(
    std::string_view file,
    int line,
    std::string_view function,
    const std::string_view message
    )
{
    volatile static bool do_assert = true;
    typedef std::pair<int, std::string> AssertLocation;
    static std::map<AssertLocation, bool> assertion_map;

    print_message_tag(
        spdlog::source_loc{file.data(), line, function.data()},
        LogLevel::Error,
        "assertion",
        fmt::format("assert ({}) failed", message)
        );

    const AssertLocation assert_location = std::make_pair(line, std::string(file));
    if (assertion_map[assert_location])
        return false;

#ifdef PORTAL_PLATFORM_WINDOWS
    if (do_assert && IsDebuggerPresent())
    {
        int res = IDTRYAGAIN;
        EndMenu();
        {
            std::thread assert_dialog(
                [&]()
                {
                    res = MessageBoxA(
                        nullptr,
                        std::format(
                            "Assert failed at:\n{}({})\n{}()\n{}\nTry again to debug, Cancel to ignore this assert in the future",
                            file,
                            line,
                            function,
                            message
                            ).c_str(),
                        "ASSERTION",
                        MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_TOPMOST
                        );
                }
                );
            assert_dialog.join();
        }

        if (res == IDCANCEL)
            // ignore this assert in the future
            assertion_map[assert_location] = true;
        else if (res == IDTRYAGAIN)
            return true; // break at outer context only if got option to select IDCANCEL
    }

    return false;

#else
    // TODO: use macos and linux windowing systems to display a message box
    return true;
#endif
}
} // namespace portal
