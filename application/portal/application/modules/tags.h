//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (LICENSE file).
//

#pragma once

#include "portal/core/flags.h"

namespace portal
{
/**
 * Bitflag enumeration of module lifecycle phases.
 *
 * ModuleTags defines the available lifecycle hooks that modules can participate in.
 * Each tag corresponds to a specific method in BaseModule (begin_frame, update, etc.).
 * Modules declare which lifecycle phases they need by specifying one or more of these
 * tags in their TaggedModule template parameter.
 * 
 * Lifecycle execution order within each frame:
 * 1. FrameLifecycle - begin_frame() called in dependency order
 * 2. Update - update() called in dependency order
 * 3. GuiUpdate - gui_update() called in dependency order
 * 4. PostUpdate - post_update() called in dependency order (typically rendering)
 * 5. FrameLifecycle - end_frame() called in reverse dependency order
 */
enum class ModuleTags: uint8_t
{
    None           = 0b00000000,
    FrameLifecycle = 0b00000001,
    Update         = 0b00000010,
    GuiUpdate      = 0b00000100,
    PostUpdate     = 0b00001000
};

/**
 * Type alias for ModuleTags bitflag wrapper.
 * Provides type-safe bitwise operations on ModuleTags.
 */
using TagFlag = Flags<ModuleTags>;

/**
 * Flag traits specialization enabling bitwise operations on ModuleTags.
 * Configures the Flags template to treat ModuleTags as a bitmask enumeration.
 */
template <>
struct FlagTraits<ModuleTags>
{
    using enum ModuleTags;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = FrameLifecycle | Update | GuiUpdate | PostUpdate;
};

/**
 * Compile-time tag combination template for module lifecycle declaration.
 *
 * Tag is a variadic template that combines multiple ModuleTags into a single
 * compile-time computed bitflag. Modules use this template as the first parameter
 * to TaggedModule to declare which lifecycle phases they participate in.
 *
 * Usage examples:
 * @code
 * // Module that only updates, no frame lifecycle hooks
 * class GameLogic : public TaggedModule<Tag<ModuleTags::Update>, ECS> { };
 *
 * // Module that participates in frame lifecycle and rendering
 * class Renderer : public TaggedModule<Tag<ModuleTags::FrameLifecycle, ModuleTags::PostUpdate>> { };
 *
 * // Module that handles events and updates
 * class InputHandler : public TaggedModule<Tag<ModuleTags::Event, ModuleTags::Update>> { };
 *
 * // Module with no lifecycle tags (dependency-only)
 * class ResourceRegistry : public TaggedModule<Tag<>> { };
 * @endcode
 *
 * @tparam TagBits Variadic pack of ModuleTags to combine into this tag set
 */
template <ModuleTags... TagBits>
class Tag
{
public:
    // MSVC fails to automatically deduce `operator|` on the enum... :(
    constexpr static auto Tags = (TagFlag{ModuleTags::None} | ... | TagFlag{TagBits});

    /**
     * Check if this tag set contains a specific tag.
     *
     * @param tag The TagFlag to check for membership
     * @return true if the tag is present in this Tag's bitflag set
     */
    constexpr static bool has_tag(const TagFlag& tag)
    {
        return (Tags & tag) != ModuleTags::None;
    }

    /**
     * Check if this tag set contains a specific tag.
     *
     * @tparam T The ModuleTags value to check for membership
     * @return true if the tag is present in this Tag's bitflag set
     */
    template <ModuleTags T>
    constexpr static bool has_tag()
    {
        return has_tag(Tag<T>::Tags);
    }

    /**
     * Check if this tag set contains all specified tags.
     *
     * Uses fold expressions to verify all tags are present.
     *
     * @tparam C Variadic pack of ModuleTags to check for membership
     * @return true if all specified tags are present in this Tag's bitflag set
     */
    template <ModuleTags... C>
    constexpr static bool has_tags()
    {
        return (has_tag<C>() && ...);
    }
};


/**
 * Concept requiring a type to support tag membership queries.
 *
 * TagConcept constrains template parameters to types that provide a has_tag()
 * method accepting a TagFlag and returning a boolean. This is satisfied by
 * the Tag template and is used to constrain TaggedModule's template parameters,
 * ensuring compile-time verification of tag declarations.
 *
 * @tparam T Type to check for tag concept compliance
 */
template <typename T>
concept TagConcept = requires(TagFlag tag) {
    { T::has_tag(tag) } -> std::convertible_to<bool>;
};
}
