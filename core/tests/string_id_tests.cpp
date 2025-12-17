//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/strings/string_id.h"

#include <gtest/gtest.h>

namespace portal
{
using namespace testing;
using namespace std::literals;

TEST(StringIdTest, constexpr_construction)
{
    [[maybe_unused]] constexpr StringId def;
    [[maybe_unused]] constexpr StringId hash_only(434649463043236531ull);
    [[maybe_unused]] StringId name_hash(434649463043236531ull, "root"sv);
    [[maybe_unused]] constexpr auto using_define = STRING_ID("root");

    EXPECT_EQ(hash_only, name_hash);
    EXPECT_EQ(name_hash, using_define);
}
}
