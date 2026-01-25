//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <entt/entt.hpp>
#include <glaze/reflection/get_name.hpp>

#include "portal/engine/ecs/entity.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/serialization/archive.h"
#include "portal/serialization/serialize.h"


namespace portal::ecs
{
namespace details
{
    template <typename T>
    concept FindDependencies = requires(T t) {
        { t.get_dependencies() } -> std::same_as<std::vector<StringId>>;
    };

    template <typename T>
    concept PostSerializationPass = requires(T t, Entity entity, ResourceRegistry& resource_registry) {
        { t.post_serialization(entity, resource_registry) } -> std::same_as<void>;
    };

    template <typename T>
    concept ArchiveableComponentConcept = requires(T t, ArchiveObject& s, Entity entity, ecs::Registry& ecs_reg) {
        { t.archive(s, entity, ecs_reg) } -> std::same_as<void>;
    };

    template <typename T>
    concept DearchiveableComponentConcept = requires(ArchiveObject& d, Entity entity, ecs::Registry& ecs_reg) {
        { T::dearchive(d, entity, ecs_reg) } -> std::same_as<T>;
    };

    template <typename T>
    concept SerializableComponentConcept = requires(const T t, Serializer& s, Entity entity, ecs::Registry& ecs_reg) {
        { t.serialize(s, entity, ecs_reg) } -> std::same_as<void>;
    };

    template <typename T>
    concept DeserializableComponentConcept = requires(Deserializer& d, Entity entity, ecs::Registry& ecs_reg) {
        { T::deserialize(d, entity, ecs_reg) } -> std::same_as<T>;
    };
}

template <typename T>
static void archive_component(Entity entity, ArchiveObject& archive, [[maybe_unused]] ecs::Registry& ecs_reg)
{
    if (!entity.has_component<T>())
        return;

    if constexpr (std::is_empty_v<T>)
    {
        // Tag components have no data to archive, just mark presence
        archive.add_property(glz::type_name<T>, true);
    }
    else
    {
        const auto& comp = entity.get_component<T>();
        if constexpr (details::ArchiveableComponentConcept<T>)
        {
            auto* child = archive.create_child(glz::type_name<T>);
            comp.archive(*child, entity, ecs_reg);
        }
        else
        {
            archive.add_property(glz::type_name<T>, comp);
        }
    }
}

template <typename T>
static void dearchive_component(
    Entity entity,
    [[maybe_unused]] ArchiveObject& archive,
    [[maybe_unused]] ecs::Registry& ecs_reg
)
{
    LOG_INFO("DEARCHIVE: {}", glz::type_name<T>);
    if constexpr (std::is_empty_v<T>)
    {
        // Tag components have no data to dearchive, just add the component
        entity.add_component<T>();
    }
    else
    {
        auto inner_serialize = [](
            T& comp,
            [[maybe_unused]] Entity& entity,
            [[maybe_unused]] ArchiveObject& archive,
            [[maybe_unused]] ecs::Registry& ecs_reg
        )
        {
            if constexpr (details::DearchiveableComponentConcept<T>)
            {
                auto* child = archive.get_object(glz::type_name<T>);
                comp = T::dearchive(*child, entity, ecs_reg);
            }
            else
            {
                archive.get_property(glz::type_name<T>, comp);
            }
        };

        if (entity.has_component<T>())
        {
            T& comp = entity.get_component<T>();
            inner_serialize(comp, entity, archive, ecs_reg);
        }
        else
        {
            T& comp = entity.add_component<T>();
            inner_serialize(comp, entity, archive, ecs_reg);
        }
    }
}

template <typename T>
static void serialize_component(
    Entity entity,
    Serializer& serializer,
    [[maybe_unused]] ecs::Registry& ecs_reg
)
{
    if (!entity.has_component<T>())
        return;

    serializer.add_value(STRING_ID(glz::type_name<T>));
    if constexpr (std::is_empty_v<T>)
    {
        // Tag components have no data to serialize, just mark presence
        serializer.add_value(true);
    }
    else
    {
        const auto& comp = entity.get_component<T>();
        if constexpr (details::SerializableComponentConcept<T>)
        {
            comp.serialize(serializer, entity, ecs_reg);
        }
        else
        {
            serializer.add_value(comp);
        }
    }
}

template <typename T>
static void deserialize_component(
    Entity entity,
    Deserializer& deserializer
)
{
    if constexpr (std::is_empty_v<T>)
    {
        // Tag components have no data to deserialize, just add the component
        [[maybe_unused]] bool present{};
        deserializer.get_value(present);
        entity.add_component<T>();
    }
    else
    {
        T out_component{};
        deserializer.get_value(out_component);
        entity.patch_or_add_component<T>(std::move(out_component));
    }
}

template <typename T>
static void post_serialization_pass(Entity entity, ResourceRegistry& reg)
{
    if (!entity.has_component<T>())
        return;

    if constexpr (std::is_empty_v<T>)
    {
        T comp;
        if constexpr (details::PostSerializationPass<T>)
        {
            comp.post_serialization(entity, reg);
        }
    }
    else
    {
        auto& comp = entity.get_component<T>();
        if constexpr (details::PostSerializationPass<T>)
        {
            comp.post_serialization(entity, reg);
        }
    }
}

template <typename T>
static std::vector<StringId> find_dependencies(Entity entity)
{
    if (!entity.has_component<T>())
        return {};

    if constexpr (std::is_empty_v<T>)
    {
        T comp;
        if constexpr (details::FindDependencies<T>)
        {
            return comp.get_dependencies();
        }
        else
        {
            return std::vector<StringId>{};
        }
    }
    else
    {
        auto& comp = entity.get_component<T>();
        if constexpr (details::FindDependencies<T>)
        {
            return comp.get_dependencies();
        }
        else
        {
            return std::vector<StringId>{};
        }
    }
}

template <typename T>
static void print(Entity entity)
{
    if (!entity.has_component<T>())
        return;

    if constexpr (fmt::is_formattable<T>())
    {
        LOG_INFO_TAG("ECS", "  {}", entity.get_component<T>());
    }
    else if constexpr (!std::is_empty_v<T> && glz::reflectable<T>)
    {
        LOG_INFO_TAG("ECS", "  {}", glz::type_name<T>);
        auto& comp = entity.get_component<T>();
        constexpr auto N = glz::reflect<T>::size;
        if constexpr (N > 0)
        {
            glz::for_each<N>(
                [&]<size_t I>()
                {
                    LOG_INFO_TAG(
                        "ECS",
                        "    {} -> {}",
                        glz::reflect<T>::keys[I],
                        glz::get_member(comp, glz::get<I>(glz::to_tie(comp)))
                    );
                }
            );
        }
    }
}


template <typename T>
void register_component()
{
    using namespace entt::literals;

    entt::meta_factory<T>()
        .type(static_cast<entt::id_type>(STRING_ID(glz::type_name<T>).id), STRING_ID(glz::type_name<T>).string.data())
        .template func<&archive_component<T>>(static_cast<entt::id_type>(STRING_ID("archive").id))
        .template func<&dearchive_component<T>>(static_cast<entt::id_type>(STRING_ID("dearchive").id))
        .template func<&serialize_component<T>>(static_cast<entt::id_type>(STRING_ID("serialize").id))
        .template func<&deserialize_component<T>>(static_cast<entt::id_type>(STRING_ID("deserialize").id))
        .template func<&post_serialization_pass<T>>(static_cast<entt::id_type>(STRING_ID("post_serialization").id))
        .template func<&print<T>>(static_cast<entt::id_type>(STRING_ID("print").id))
        .template func<&find_dependencies<T>>(static_cast<entt::id_type>(STRING_ID("find_dependencies").id));
}

#define REGISTER_COMPONENT(ComponentType) \
namespace { \
struct ComponentType##_registrar { \
ComponentType##_registrar() { \
::portal::ecs::register_component<ComponentType>(); \
} \
}; \
inline ComponentType##_registrar ComponentType##_registrar_instance; \
}

REGISTER_COMPONENT(PlayerTag);
}
