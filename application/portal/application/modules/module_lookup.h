//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <algorithm>

#include "module_stack.h"

namespace portal
{
template <typename... Modules>
class ModuleLookup
{
public:
    using ModulesTuple = std::tuple<Modules&...>;

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

    [[nodiscard]] std::optional<ModulesTuple> get_modules() const { return modules.value(); }

private:
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
