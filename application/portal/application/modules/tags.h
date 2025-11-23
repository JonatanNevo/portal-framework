//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (LICENSE file).
//

#pragma once
#include <tuple>

#include "portal/core/flags.h"

namespace portal
{
enum class ModuleTagBits: uint8_t
{
    None                 = 0b00000000,
    FrameLifecycle       = 0b00000001,
    FrameContextCreation = 0b00000010,
    Update               = 0b00000100,
    GuiUpdate            = 0b00001000,
    PostUpdate           = 0b00010000,
    Input                = 0b00100000,
    Event                = 0b01000000
};

using ModuleTags = Flags<ModuleTagBits>;

template <>
struct FlagTraits<ModuleTagBits>
{
    using enum ModuleTagBits;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = FrameLifecycle | FrameContextCreation | Update | GuiUpdate | PostUpdate | Input | Event;
};

using TagID = const void*;

template <typename... Tags>
class Tag
{
    constexpr static int id_holder = 0;

public:
    constexpr static TagID ID = &id_holder;

    constexpr static bool has_tag(const TagID& tag)
    {
        return has_tag_impl<0>(tag);
    }

    template <typename T>
    constexpr static bool has_tag()
    {
        return has_tag(Tag<T>::ID);
    }

    template <typename... C>
    constexpr static bool has_tags()
    {
        return (has_tag<C>() && ...);
    }

private:
    constexpr static std::array<TagID, sizeof...(Tags)> tags = {Tag<Tags>::ID...};

    template <std::size_t I = 0>
    constexpr static bool has_tag_impl(const TagID& tag)
    {
        if constexpr (I >= sizeof...(Tags))
        {
            return false;
        }
        else
        {
            if (tags[I] == tag)
                return true;
            return has_tag_impl<I + 1>(tag);
        }
    }
};


template <typename T>
concept TagConcept = requires(TagID id) {
    { T::has_tag(id) } -> std::convertible_to<bool>;
};
}
