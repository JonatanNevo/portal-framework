//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "deletion_queue.h"

namespace portal
{

void DeletionQueue::push_deleter(std::function<void()>&& deleter)
{
    deletion_queue.push_back(deleter);
}

//void DeletionQueue::push_move_deleter(std::move_only_function<void()> deleter)
//{
//    move_deletion_queue.emplace_back(std::move(deleter));
//}


void DeletionQueue::flush()
{
    // reverse iterate the deletion queue to execute all the functions
    // for (auto it = move_deletion_queue.rbegin(); it != move_deletion_queue.rend(); ++it)
    // {
    //     (*it)();
    // }

    for (auto it = deletion_queue.rbegin(); it != deletion_queue.rend(); ++it)
    {
        (*it)();
    }

    deletion_queue.clear();
}

} // portal
