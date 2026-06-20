//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>

#include "portal/core/strings/rapidhash/rapidhash.h"
#include "portal/core/strings/rapidhash/rapidhash-ct.h"

TEST_CASE("Rapidhash ct is compatible with rapidhash runtime")
{
    SECTION("Short strings")
    {
        constexpr std::string_view hello_world = "hello world";
        constexpr auto ct_hash = rapidhash_ct(hello_world.data(), hello_world.length());
        const auto runtime_hash = rapidhash(hello_world.data(), hello_world.length());
        REQUIRE(ct_hash == runtime_hash);
    }

    SECTION("Long Strings")
    {
        constexpr std::string_view very_long_string = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus euismod ligula est, at lacinia lacus laoreet vel. Nam at nunc sit amet eros porttitor placerat. Fusce eu ligula non dolor faucibus pharetra. Aliquam finibus magna aliquet, dictum leo ut, posuere nisl. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus nec lacinia orci. Fusce viverra elit vel sapien vehicula convallis. Vestibulum convallis, tortor vel porttitor porttitor, ipsum purus accumsan lectus, et facilisis sapien dolor vitae augue. Maecenas quis quam et dui mollis cursus. Donec dolor libero, suscipit varius nisi sit amet, tempus accumsan lacus. Mauris lobortis sem sed urna vehicula luctus. Maecenas vel tortor et massa facilisis semper eu in neque. Mauris mollis sed justo sed consequat. In purus turpis, interdum in pellentesque sed, egestas ac dui. Vivamus ultrices congue lacinia. Duis id sem eu lorem feugiat eleifend in aliquam odio. Sed et eros quis lectus hendrerit volutpat. Nulla fermentum finibus convallis. Fusce ac et.";
        constexpr auto ct_hash = rapidhash_ct(very_long_string.data(), very_long_string.length());
        const auto runtime_hash = rapidhash(very_long_string.data(), very_long_string.length());
        REQUIRE(ct_hash == runtime_hash);
    }
}