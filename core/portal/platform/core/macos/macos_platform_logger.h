//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>
#include <spdlog/spdlog.h>

namespace portal::platform
{
const std::vector<spdlog::sink_ptr>& get_platform_sinks(const std::filesystem::path& logging_folder);

bool print_assert_dialog(std::string_view file, int line, std::string_view function, std::string_view message);
}
