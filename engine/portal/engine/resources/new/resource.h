//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/flags.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/engine/strings/string_id.h"

namespace portal
{
enum class ResourceDirtyBits : uint8_t
{
    Clean        = 0b00000000,
    StateChange  = 0b00000001,
    DataChange   = 0b00000010,
    ConfigChange = 0b00000100,
};

using ResourceDirtyFlags = Flags<ResourceDirtyBits>;

template<>
struct FlagTraits<ResourceDirtyBits>
{
    static constexpr bool is_bitmask = true;
    static constexpr Flags<ResourceDirtyBits> all_flags = ResourceDirtyBits::StateChange | ResourceDirtyBits::DataChange | ResourceDirtyBits::ConfigChange;
};
}

namespace portal::ng
{

using ResourceHandle = size_t;
constexpr ResourceHandle INVALID_RESOURCE_HANDLE = 0;

class Resource
{
public:
    explicit Resource(const StringId& id) : id(id) {}
    virtual ~Resource() = default;

    static StringId static_type_name() { return STRING_ID("Resource"); }

    [[nodiscard]] const StringId& get_id() const { return id; }

private:
    StringId id;
};

template <typename T>
concept ResourceConcept = requires {
    { T::static_type_name() } -> std::same_as<StringId>;
    std::is_base_of_v<Resource, T>;
};

} // portal
