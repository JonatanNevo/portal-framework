//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{

class Queue
{
public:
    virtual ~Queue() = default;

    [[nodiscard]] virtual size_t get_index() const = 0;

    // TODO: add `submit` `present`
};

}