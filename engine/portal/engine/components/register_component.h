//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <entt/entt.hpp>
#include <glaze/reflection/get_name.hpp>

#include "portal/engine/ecs/entity.h"
#include "portal/engine/ecs/registry.h"
#include "portal/serialization/archive.h"
#include "portal/serialization/serialize.h"

namespace portal::ecs
{
namespace details
{
    template <typename T>
    concept ArchiveableComponentConcept = requires(T t, ArchiveObject& s, Entity entity, ecs::Registry& reg) {
        { t.archive(s, entity, reg) } -> std::same_as<void>;
    };

    template <typename T>
    concept DearchiveableComponentConcept = requires(ArchiveObject& d, Entity entity, ecs::Registry& reg) {
        { T::dearchive(d, entity, reg) } -> std::same_as<T>;
    };

    template <typename T>
    concept SerializableComponentConcept = requires(const T t, Serializer& s, Entity entity, ecs::Registry& reg) {
        { t.serialize(s, entity, reg) } -> std::same_as<void>;
    };

    template <typename T>
    concept DeserializableComponentConcept = requires(Deserializer& d, Entity entity, ecs::Registry& reg) {
        { T::deserialize(d, entity, reg) } -> std::same_as<void>;
    };
}

template <typename T>
static void archive_component(Entity entity, ArchiveObject& archive, [[maybe_unused]] ecs::Registry& reg)
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
            comp.archive(*child, entity, reg);
        }
        else
        {
            archive.add_property(glz::type_name<T>, comp);
        }
    }
}

template <typename T>
static void dearchive_component(Entity entity, [[maybe_unused]] ArchiveObject& archive, [[maybe_unused]] ecs::Registry& reg)
{
    if constexpr (std::is_empty_v<T>)
    {
        // Tag components have no data to dearchive, just add the component
        entity.add_component<T>();
    }
    else
    {
        T out_component{};
        if constexpr (details::DearchiveableComponentConcept<T>)
        {
            auto* child = archive.get_object(glz::type_name<T>);
            out_component = T::dearchive(*child, entity, reg);
        }
        else
        {
            archive.get_property(glz::type_name<T>, out_component);
        }
        entity.add_component<T>(std::move(out_component));
    }
}

template <typename T>
static void serialize_component(Entity entity, Serializer& serializer, [[maybe_unused]] ecs::Registry& reg)
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
            comp.serialize(serializer, entity, reg);
        }
        else
        {
            serializer.add_value(comp);
        }
    }
}

template <typename T>
static void deserialize_component(Entity entity, [[maybe_unused]] Deserializer& deserializer, [[maybe_unused]] ecs::Registry& reg)
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
        if constexpr (details::DeserializableComponentConcept<T>)
        {
            T::deserialize(out_component, deserializer, entity, reg);
        }
        else
        {
            deserializer.get_value(out_component);
        }
        entity.add_component<T>(std::move(out_component));
    }
}


template <typename T>
void register_component()
{
    using namespace entt::literals;

    entt::meta_factory<T>()
        .type(entt::type_hash<T>::value(), STRING_ID(glz::type_name<T>).string.data())
        .template func<&archive_component<T>>(STRING_ID("archive").id)
        .template func<&dearchive_component<T>>(STRING_ID("dearchive").id)
        .template func<&serialize_component<T>>(STRING_ID("serialize").id)
        .template func<&deserialize_component<T>>(STRING_ID("deserialize").id);
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
