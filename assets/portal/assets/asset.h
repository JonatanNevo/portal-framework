//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once
#include <portal/core/uuid.h>
#include <portal/core/reference.h>

#include "portal/assets/asset_types.h"

namespace portal
{
using AssetHandle = UUID;

class Asset: public CountedReference
{
public:
    AssetHandle handle = 0;
    std::underlying_type_t<AssetFlag> flags = static_cast<std::underlying_type_t<AssetFlag>>(AssetFlag::Empty);

    virtual ~Asset() = default;

    static AssetType get_static_type() { return AssetType::Undefined; }
    [[nodiscard]] virtual AssetType get_asset_type() const { return AssetType::Undefined; }

    virtual void on_dependency_updated(AssetHandle handle) {};

    virtual bool operator==(const Asset& other) const { return handle == other.handle; }
    virtual bool operator!=(const Asset& other) const { return handle != other.handle; }

private:
    friend class AssetManager;

    [[nodiscard]] bool is_valid() const
    {
        return (
            flags & static_cast<std::underlying_type_t<AssetFlag>>(AssetFlag::Missing)
            | flags & static_cast<std::underlying_type_t<AssetFlag>>(AssetFlag::Invalid)
        ) == 0;
    }

    [[nodiscard]] bool is_flag_set(AssetFlag flag) const
    {
        return static_cast<uint8_t>(flag) & flags;
    }

    void set_flag(AssetFlag flag, bool value = true)
    {
        if (value)
            flags |= static_cast<std::underlying_type_t<AssetFlag>>(flag);
        else
            flags &= ~static_cast<std::underlying_type_t<AssetFlag>>(flag);
    }
};

template <typename T> requires std::is_base_of_v<Asset, T>
struct AsyncAssetResult
{
    Reference<T> asset;
    bool is_ready = false;

    AsyncAssetResult() = default;
    AsyncAssetResult(const AsyncAssetResult& other) = default;

    explicit AsyncAssetResult(Reference<T> asset, const bool is_ready = false): asset(asset), is_ready(is_ready) {}

    template <typename OtherT> requires std::is_base_of_v<Asset, OtherT>
    explicit AsyncAssetResult(const AsyncAssetResult<OtherT>& other): asset(other.asset.template as<T>()), is_ready(other.is_ready) {};

    explicit operator Reference<T>() const { return asset; }
    explicit operator bool() const { return is_ready; }
};
} // portal
