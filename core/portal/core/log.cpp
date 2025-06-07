//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>
#include <map>
#include <ranges>
#include <utility>

#define PORTAL_HAS_CONSOLE !PORTAL_DIST


#if defined(PORTAL_ASSERT_MESSAGE_BOX) && !defined(PORTAL_DIST)
#ifdef PORTAL_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif


std::thread::id portal::Log::main_thread_id;

namespace portal
{
std::shared_ptr<spdlog::logger> Log::core_logger;
std::shared_ptr<spdlog::logger> Log::client_logger;

LogExtra::~LogExtra()
{
    for (const auto& key : pairs | std::views::keys)
    {
        spdlog::mdc::remove(key);
    }
}

void Log::init()
{
    main_thread_id = std::this_thread::get_id();

    const std::string log_directory = "logs";
    if (!std::filesystem::exists(log_directory))
        std::filesystem::create_directory(log_directory);

    std::vector<spdlog::sink_ptr> core_sinks = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/portal.log", true),
#if PORTAL_HAS_CONSOLE
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
    };

    std::vector<spdlog::sink_ptr> client_sink = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/client.log", true),
#if PORTAL_HAS_CONSOLE
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
    };

    core_sinks[0]->set_pattern("[%T] [%l] %v");
    client_sink[0]->set_pattern("[%T] [%l] %v");

#if PORTAL_HAS_CONSOLE
    core_sinks[1]->set_pattern("%^[%T] %n: %v%$");
    client_sink[1]->set_pattern("%^[%T] %n: %v%$");
#endif

    core_logger = std::make_shared<spdlog::logger>("core", begin(core_sinks), end(core_sinks));
    core_logger->set_level(spdlog::level::trace);

    client_logger = std::make_shared<spdlog::logger>("client", begin(client_sink), end(client_sink));
    client_logger->set_level(spdlog::level::trace);
}

void Log::shutdown()
{
    client_logger.reset();
    core_logger.reset();
    spdlog::drop_all();
}

bool Log::print_assert_message(
    const LoggerType type,
    std::string_view file,
    int line,
    std::string_view function,
    const std::string_view message
)
{
    volatile static bool do_assert = true;
    typedef std::pair<int, std::string> AssertLocation;
    static std::map<AssertLocation, bool> assertion_map;

    print_message_tag(type, LogLevel::Error, "ASSERTION", std::format("assert ({}) failed", message));

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
