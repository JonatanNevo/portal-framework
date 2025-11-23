//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (LICENSE file).
//

#pragma once

#include "portal/core/flags.h"

namespace portal
{
enum class ModuleTags: uint8_t
{
    None           = 0b00000000,
    FrameLifecycle = 0b00000001,
    Update         = 0b00000010,
    GuiUpdate      = 0b00000100,
    PostUpdate     = 0b00001000,
    Event          = 0b00010000
};

using TagFlag = Flags<ModuleTags>;

template <>
struct FlagTraits<ModuleTags>
{
    using enum ModuleTags;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = FrameLifecycle | Update | GuiUpdate | PostUpdate | Event;
};

template <ModuleTags... TagBits>
class Tag
{
public:
    // MSVC fails to automatically deduce `operator|` on the enum... :(
    constexpr static auto Tags = (TagFlag{ModuleTags::None} | ... | TagFlag{TagBits});

    constexpr static bool has_tag(const TagFlag& tag)
    {
        return (Tags & tag) != ModuleTags::None;
    }

    template <ModuleTags T>
    constexpr static bool has_tag()
    {
        return has_tag(Tag<T>::Tags);
    }

    template <ModuleTags... C>
    constexpr static bool has_tags()
    {
        return (has_tag<C>() && ...);
    }
};


template <typename T>
concept TagConcept = requires(TagFlag tag) {
    { T::has_tag(tag) } -> std::convertible_to<bool>;
};
}
