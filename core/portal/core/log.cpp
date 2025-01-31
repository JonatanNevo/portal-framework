//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>

#define PORTAL_HAS_CONSOLE !PORTAL_DIST

namespace portal
{
    std::shared_ptr<spdlog::logger> Log::core_logger;
    std::shared_ptr<spdlog::logger> Log::client_logger;

    void Log::init()
    {
        std::string log_directory = "logs";
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


} // namespace portal
