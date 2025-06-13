//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace portal
{

struct StringId
{
    uint64_t id = 0;
    std::string_view string;
};

constexpr static StringId INVALID_STRING_ID = { 0, std::string_view("Invalid")};

class StringIdPool
{
public:
    StringId store(std::string_view string);
    StringId find(std::string_view string);

private:
    std::unordered_map<uint64_t, std::string> entries{};
};

} // portal
