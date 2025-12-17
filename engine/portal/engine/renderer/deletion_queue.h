//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <deque>
#include <functional>


namespace portal
{
class DeletionQueue
{
public:
    void push_deleter(std::function<void()>&& deleter);
    void flush();

private:
    std::deque<std::function<void()>> deletion_queue;
};
} // portal
