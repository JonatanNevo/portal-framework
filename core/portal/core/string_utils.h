//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace portal
{

std::vector<std::string> split(const std::string_view string, const std::string_view& delimiters);
std::vector<std::string> split(const std::string_view string, const char delimiter);

}
