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
    [[maybe_unused]] constexpr StringId hash_only(12345);
    [[maybe_unused]] StringId name_hash(12345, "something"sv);


    constexpr auto view = StringRegistry::find_constexpr(12345);
    EXPECT_EQ(view, "hello");
}

}