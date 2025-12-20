//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <portal/core/strings/string_id.h>

#include "portal/application/modules/base_module.h"
#include "portal/application/modules/module_lookup.h"
#include "module_stack.h"

namespace portal
{
/**
 * CRTP template implementing compile-time dependency injection for modules.
 *
 * The first template parameter declares which lifecycle phases this module participates
 * in using the Tag<ModuleTags...> template. The remaining variadic parameters list
 * other modules this module depends on. During registration, TaggedModule uses
 * ModuleLookup to resolve these dependencies and stores them as references in a tuple.
 *
 * Derived modules access dependencies using get_dependency<T>(), which compiles to
 * direct tuple access with no runtime cost. The ModuleStack uses the tag information
 * to organize modules into efficient execution lists sorted by dependency level.
 *
 * Example - Module with dependencies:
 * @code
 * class MyGameModule : public TaggedModule<
 *     Tag<ModuleTags::Update>,
 *     Renderer,        // Dependencies
 *     ResourceRegistry
 * > {
 *     void update(FrameContext& frame) override {
 *         auto& renderer = get_dependency<Renderer>();
 *         auto& resources = get_dependency<ResourceRegistry>();
 *         // Use dependencies with zero-cost access
 *     }
 * };
 * @endcode
 *
 * @tparam Tags A Tag<ModuleTags...> type declaring lifecycle phase participation
 * @tparam Dependencies Variadic pack of module types this module depends on
 */
template <TagConcept Tags, typename... Dependencies>
class TaggedModule : public BaseModule
{
public:
    /**
     * Construct module and resolve dependencies.
     * @param stack The ModuleStack to search for dependencies
     * @param name Module name for debugging
     * @throws std::runtime_error if any dependency cannot be found
     */
    explicit TaggedModule(ModuleStack& stack, const StringId& name)
        : BaseModule(name), dependencies(create_dependencies(stack))
    {}

    /** @return Vector of module pointers this module depends on */
    [[nodiscard]] std::vector<BaseModule*> get_dependencies() const override
    {
        if constexpr (sizeof...(Dependencies) == 0)
        {
            return {};
        }
        else
        {
            return [this]<size_t... Is>(std::index_sequence<Is...>)
            {
                return std::vector<BaseModule*>{&std::get<Is>(dependencies)...};
            }(std::index_sequence_for<Dependencies...>{});
        }
    }

    /**
     * Compile-time check if this module has a specific tag.
     * @tparam T The ModuleTags value to check
     * @return true if this module participates in the specified lifecycle phase
     */
    template <ModuleTags T>
    consteval static bool has_tag_static()
    {
        return ModuleTags::has_tag(Tag<T>::ID);
    }

    /**
     * Compile-time check if this module has all specified tags.
     * @tparam T Variadic pack of ModuleTags to check
     * @return true if this module participates in all specified lifecycle phases
     */
    template <ModuleTags... T>
    consteval static bool has_tags_static()
    {
        return ModuleTags::template has_tags<T...>();
    }

    using BaseModule::has_tag;
    using BaseModule::has_tags;

    /**
     * Runtime check if this module has a specific tag.
     * @param tag The TagFlag to check
     * @return true if this module participates in the specified lifecycle phase
     */
    [[nodiscard]] bool has_tag(TagFlag tag) const override
    {
        return ModuleTags::has_tag(tag);
    }

protected:
    /**
     * Access a dependency by type (zero-cost abstraction).
     * @tparam T The module type to retrieve
     * @return Reference to the dependency module
     */
    template <typename T>
    T& get_dependency()
    {
        return std::get<T&>(dependencies);
    }

    /**
     * Access a dependency by type (const version).
     * @tparam T The module type to retrieve
     * @return Const reference to the dependency module
     */
    template <typename T>
    const T& get_dependency() const
    {
        return std::get<T&>(dependencies);
    }

private:
    auto create_dependencies(ModuleStack& registry)
    {
        ModuleLookup<Dependencies...> lookup(registry);
        if (!lookup.get_modules().has_value())
        {
            throw std::runtime_error("Failed to resolve all dependencies");
        }
        return lookup.get_modules().value();
    }

protected:
    using ModuleTags = Tags;
    std::tuple<Dependencies&...> dependencies;  ///< Tuple of references to dependency modules
};

/**
 * Convenience alias for modules with no lifecycle tags (dependency-only modules).
 *
 * Use Module<Dependencies...> when a module exists only to provide services to
 * other modules and doesn't need to participate in any lifecycle hooks.
 *
 * Example:
 * @code
 * class ResourceRegistry : public Module<FileSystem, Allocator> {
 *     // No lifecycle methods, just provides resource lookup service
 * };
 * @endcode
 *
 * @tparam Dependencies Variadic pack of module types this module depends on
 */
template <typename... Dependencies>
using Module = TaggedModule<Tag<>, Dependencies...>;
}
