//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/log.h"
#include "portal/engine/allocators/pool_allocator.h"
#include "portal/engine/strings/md5_hash.h"

int main()
{
    portal::Log::init();

    using namespace portal::hash::literals;

    auto hash = portal::hash::md5("hello");
    auto hash2 = "hello"_md5;
    LOG_INFO("{:x}", hash);
    LOG_INFO("{:x}", hash2);
}
