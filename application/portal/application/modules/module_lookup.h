//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <algorithm>

#include "module_stack.h"

namespace portal
{
/**
 * Helper template for resolving module dependencies during registration.
 *
 * ModuleLookup searches the ModuleStack for all specified module types and returns
 * a tuple of references to them. This is used by TaggedModule during construction
 * to resolve its variadic Dependencies... pack into concrete module references.
 *
 * This performs the dependency resolution once at registration time, enabling
 * zero-cost access during the game loop.
 *
 * @tparam Modules Variadic pack of module types to look up
 */
template <typename... Modules>
class ModuleLookup
{
public:
    using ModulesTuple = std::tuple<Modules&...>;

    /**
     * Construct a lookup and search for all specified module types.
     * @param module_stack The ModuleStack to search for dependencies
     */
    explicit ModuleLookup(ModuleStack& module_stack)
    {
        std::vector<BaseModule*> found_modules(std::tuple_size_v<ModulesTuple>, nullptr);

        [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            (find_module<Is, std::tuple_element_t<Is, std::tuple<Modules...>>>(
                found_modules,
                module_stack.list_modules()
            ), ...);
        }(std::index_sequence_for<Modules...>{});

        if (std::ranges::all_of(found_modules, [](auto* m) { return m != nullptr; }))
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                modules.emplace(
                    *static_cast<std::tuple_element_t<Is, std::tuple<Modules...>>*>(found_modules[Is])...
                );
            }(std::index_sequence_for<Modules...>{});
        }
    }

    /**
     * Get the resolved module dependencies.
     * @return Optional tuple of module references, empty if any dependency was not found
     */
    [[nodiscard]] std::optional<ModulesTuple> get_modules() const { return modules; }

private:
    /**
     * Search for a specific module type using dynamic_cast.
     * @tparam Index The index in the tuple for this module type
     * @tparam ModuleType The concrete module type to search for
     * @param found Vector to store found module pointers
     * @param module_stack_modules The list of registered modules to search
     */
    template <size_t Index, typename ModuleType>
    void find_module(std::vector<BaseModule*>& found, const auto& module_stack_modules)
    {
        for (auto& module : module_stack_modules)
        {
            if (auto* typed_module = dynamic_cast<ModuleType*>(module.get()))
            {
                found[Index] = typed_module;
                break;
            }
        }
    }

    std::optional<ModulesTuple> modules = std::nullopt;
};
}
