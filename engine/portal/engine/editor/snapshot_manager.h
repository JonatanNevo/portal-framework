//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <array>

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
        StringId title;
        Buffer data;
    };

    explicit SnapshotManager(ResourceRegistry& registry);

    void set_scene_id(const StringId& new_scene_id);
    void prepare_snapshot(const StringId& title);
    void commit_snapshot();
    void revert_snapshot(std::optional<size_t> snapshot_index = std::nullopt);

private:
    StringId scene_id;
    ResourceRegistry& registry;

    SnapshotData in_flight_snapshot;
    std::array<SnapshotData, MAX_SNAPSHOTS> snapshots;
    size_t current_snapshot = 0;
};

} // portal
