//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "asset_types.h"
#include "portal/core/buffer.h"

namespace portal
{

class AssetSource;

class Asset
{
public:
    virtual ~Asset()
    {
        if (data && data.is_allocated())
            data.release();
    }

    [[nodiscard]] bool is_valid() const { return state == AssetState::Loaded; }
    [[nodiscard]] virtual AssetType get_type() const = 0;
    [[nodiscard]] AssetState get_state() const { return state; }
    [[nodiscard]] AssetSource* get_source() const { return source.get(); }

protected:
    friend class AssetManager;
    virtual void set_data(Buffer new_data) = 0;

    AssetState state = AssetState::Invalid;
    Buffer data = nullptr;
    std::shared_ptr<AssetSource> source;
};

} // namespace portal::assets
