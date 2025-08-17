//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "macos_platform_logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "portal/core/debug/assert.h"

#define PORTAL_HAS_CONSOLE !PORTAL_DIST

namespace portal::platform
{

const std::vector<spdlog::sink_ptr>& get_platform_sinks()
{
    static std::vector<spdlog::sink_ptr> sinks;
    if (sinks.empty())
    {
        sinks = {
            std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/portal.log", true),
    #if PORTAL_HAS_CONSOLE
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
    #endif
        };
    }

    return sinks;
}

bool print_assert_dialog(std::string_view, int, std::string_view, std::string_view)
{
    //TODO: use mac windowing system to popup an assert box
    return false;
}

}
