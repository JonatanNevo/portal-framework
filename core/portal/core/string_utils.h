//
// Created by Jonatan Nevo on 25/02/2025.
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
