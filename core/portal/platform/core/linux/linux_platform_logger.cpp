//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "linux_platform_logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "portal/core/debug/assert.h"
#include "portal/core/files/file_system.h"


namespace portal::platform
{
const std::vector<spdlog::sink_ptr>& get_platform_sinks(const std::filesystem::path& logging_folder)
{
    static std::vector<spdlog::sink_ptr> sinks;
    if (sinks.empty())
    {
        sinks = {
            std::make_shared<spdlog::sinks::basic_file_sink_mt>((logging_folder/ "portal.log").generic_string(), true),
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
        };
    }

    return sinks;
}

bool print_assert_dialog(std::string_view, int, std::string_view, std::string_view)
{
    //TODO: use installed windowing system to popup an assert box
    return true;
}
}
