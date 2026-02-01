//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "snapshot_manager.h"

#include <spdlog/fmt/bin_to_hex.h>

#include "portal/engine/resources/resource_registry.h"
#include "portal/third_party/imgui/ImGuiNotify.h"

namespace portal
{
SnapshotManager::SnapshotManager(ResourceRegistry& registry) : registry(registry) {}

void SnapshotManager::set_scene_id(const StringId& new_scene_id)
{
    scene_id = new_scene_id;
}

void SnapshotManager::prepare_snapshot(const StringId& title)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto data = registry.snapshot(scene_id);
    auto end = std::chrono::high_resolution_clock::now();
    LOG_TRACE("Snapshot {} took {} ms", title.string.data(), std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    in_flight_snapshot.title = title;
    in_flight_snapshot.data = std::move(data);
}

void SnapshotManager::commit_snapshot()
{
    PORTAL_ASSERT(in_flight_snapshot.data != nullptr, "No snapshot to commit");
    in_flight_snapshot.timestamp = std::chrono::system_clock::now();
    snapshots[current_snapshot] = std::move(in_flight_snapshot);
    current_snapshot = get_next_snapshot_index();

    in_flight_snapshot.title = INVALID_STRING_ID;
    in_flight_snapshot.data = nullptr;
}

void SnapshotManager::revert_snapshot(const size_t snapshot_index)
{
    auto& [_, title, data] = snapshots[snapshot_index];

    if (data == nullptr)
    {
        ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "No snapshot to revert"});
        return;
    }

    registry.load_snapshot(scene_id, data);
    ImGui::InsertNotification({ImGuiToastType::Info, 3000, "Reverted %s", title.string.data()});
    current_snapshot = snapshot_index;
}

void SnapshotManager::undo()
{
    revert_snapshot(get_previous_snapshot_index());
}

void SnapshotManager::redo()
{
    // TODO: We need to capture the current state when we undo...
    revert_snapshot(get_next_snapshot_index());
}

bool SnapshotManager::can_undo() const
{
    return snapshots[get_previous_snapshot_index()].data != nullptr;
}

bool SnapshotManager::can_redo() const
{
    return snapshots[get_next_snapshot_index()].data != nullptr;
}

size_t SnapshotManager::get_next_snapshot_index() const
{
    return (current_snapshot + 1) % MAX_SNAPSHOTS;
}

size_t SnapshotManager::get_previous_snapshot_index() const
{
    return (current_snapshot - 1) % MAX_SNAPSHOTS;
}
} // portal
