//
// Created by thejo on 4/11/2025.
//

#include "asset_source.h"

#include <utility>

#include "portal/core/file_system.h"

namespace portal
{
FileAssetSource::FileAssetSource(std::filesystem::path path): path(std::move(path))
{}

portal::Buffer FileAssetSource::load_asset()
{
    if (!std::filesystem::exists(path))
    {
        LOG_WARN_TAG("Asset", "Attempting to load from invalid path: {}", path.string());
        return nullptr;
    }
    LOG_DEBUG_TAG("Asset", "Loading asset: {}", path.string());
    return portal::FileSystem::read_file_binary(path);
}

uint32_t FileAssetSource::get_id() const
{
    return static_cast<uint32_t>(std::hash<std::string>{}(path.string()));
}

NetworkAssetSource::NetworkAssetSource(std::string_view)
{
}

portal::Buffer NetworkAssetSource::load_asset()
{
    return {};
}

uint32_t NetworkAssetSource::get_id() const
{
    return 0;
}
}
