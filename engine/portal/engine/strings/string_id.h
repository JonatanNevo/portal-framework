//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/common.h"

#include "portal/engine/strings/md5_hash.h"

namespace portal
{

struct StringId
{
    uint128_t id;
    std::string_view string;

    StringId(uint128_t id);
    StringId(uint128_t id, std::string_view string);
    StringId(uint128_t id, const std::string& string);

    bool operator==(const StringId&) const;
};

} // portal

#define STRING_ID(string) portal::StringId(portal::hash::md5(string), std::string_view(string))

