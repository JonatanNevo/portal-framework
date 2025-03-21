//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once

#include <filesystem>

#include "portal/assets/asset.h"

namespace portal
{

enum class AssetLoadStatus
{
    Undefined = 0,
    Ready,
    Invalid,
    Loading
};

struct AssetMetadata
{
    AssetHandle handle;
    AssetType type;
    std::filesystem::path path;

    AssetLoadStatus status = AssetLoadStatus::Undefined;

    // TODO: add sha256 instead of last write time
    uint64_t file_last_write_time = 0;
    bool is_data_loaded = false;

    [[nodiscard]] bool is_valid() const { return handle != 0; }
 };

struct AssetLoadResponse
{
    AssetMetadata metadata;
    Reference<Asset> asset;
};

}
