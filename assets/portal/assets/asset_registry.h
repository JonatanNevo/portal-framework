//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once

#include <shared_mutex>

#include "portal/assets/asset_metadata.h"

namespace portal {

class AssetRegistry: public CountedReference
{
public:
    const AssetMetadata& get(const AssetHandle& handle);
    void set(const AssetHandle& handle, const AssetMetadata& metadata);

    size_t count() const { return registry.size(); }
    bool contains(const AssetHandle& handle);
    size_t remove(const AssetHandle& handle);
    void clear();

    auto begin() { return registry.begin(); }
    auto end() { return registry.end(); }
    [[nodiscard]] auto begin() const { return registry.cbegin(); }
    [[nodiscard]] auto end() const { return registry.cend(); }

private:
    std::unordered_map<AssetHandle, AssetMetadata> registry;
    std::mutex registry_mutex;
};

} // portal
