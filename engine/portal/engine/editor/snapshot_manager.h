//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <array>
#include <vector>

#include "portal/core/buffer.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
class ResourceRegistry;

constexpr auto MAX_SNAPSHOTS = 16;

class SnapshotManager
{
public:
    struct SnapshotData
    {
        std::chrono::system_clock::time_point timestamp;
        StringId title;
        Buffer data;
    };

    struct SnapshotView
    {
        size_t index;
        StringId title;
        std::chrono::system_clock::time_point timestamp;
    };

    explicit SnapshotManager(ResourceRegistry& registry);

    void set_scene_id(const StringId& new_scene_id);
    void prepare_snapshot(const StringId& title);
    void commit_snapshot();
    void revert_snapshot(size_t snapshot_index);

    void undo();
    void redo();

    [[nodiscard]] bool can_undo() const;
    [[nodiscard]] bool can_redo() const;

    [[nodiscard]] size_t get_current_snapshot_index() const { return current_snapshot; }

    [[nodiscard]] std::vector<SnapshotView> list_snapshots() const
    {
        std::vector<SnapshotView> result;
        for (size_t i = 0; i < snapshots.size(); ++i)
        {
            if (snapshots[i].data != nullptr)
            {
                result.push_back(SnapshotView{i, snapshots[i].title, snapshots[i].timestamp});
            }
        }
        return result;
    }

private:
    [[nodiscard]] size_t get_next_snapshot_index() const;
    [[nodiscard]] size_t get_previous_snapshot_index() const;

private:
    StringId scene_id;
    ResourceRegistry& registry;

    SnapshotData in_flight_snapshot;
    std::array<SnapshotData, MAX_SNAPSHOTS> snapshots;
    size_t current_snapshot = 0;
};
} // portal
