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
template <TagConcept Tags, typename... Dependencies>
class TaggedModule : public BaseModule
{
public:
    explicit TaggedModule(ModuleStack& stack, const StringId& name)
        : BaseModule(name), dependencies(create_dependencies(stack))
    {}

    ~TaggedModule() override = default;

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

    template <ModuleTags T>
    consteval static bool has_tag_static()
    {
        return ModuleTags::has_tag(Tag<T>::ID);
    }

    template <ModuleTags... T>
    consteval static bool has_tags_static()
    {
        return ModuleTags::template has_tags<T...>();
    }

    using BaseModule::has_tag;
    using BaseModule::has_tags;

    [[nodiscard]] bool has_tag(TagFlag tag) const override
    {
        return ModuleTags::has_tag(tag);
    }

protected:
    template <typename T>
    T& get_dependency()
    {
        return std::get<T&>(dependencies);
    }

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
    std::tuple<Dependencies&...> dependencies;
};

template <typename... Dependencies>
using Module = TaggedModule<Tag<>, Dependencies...>;
}
