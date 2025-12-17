//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <string>

namespace portal::details
{
enum class Encoding
{
    UTF8,
    ANSI
};

std::wstring to_wstring(const std::string& str, Encoding encoding = Encoding::UTF8);
}
