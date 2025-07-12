//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include "portal/core/buffer.h"

namespace portal
{

class AssetSource
{
public:
    virtual ~AssetSource() = default;
    virtual portal::Buffer load_asset() = 0;
    virtual uint32_t get_id() const = 0;
};

class FileAssetSource final : public AssetSource
{
public:
    explicit FileAssetSource(std::filesystem::path path);
    portal::Buffer load_asset() override;
    [[nodiscard]] uint32_t get_id() const override;

private:
    std::filesystem::path path;
};

class NetworkAssetSource final : public AssetSource
{
public:
    NetworkAssetSource(std::string_view url);
    portal::Buffer load_asset() override;
    uint32_t get_id() const override;

private:
    std::string_view url;
};

}
