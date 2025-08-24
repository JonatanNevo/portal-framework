//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <concepts>
#include <string>
#include <variant>

#include <portal/core/buffer.h>
#include <portal/core/reflection/property.h>

#include "portal/serialization/archive.h"
#include "portal/serialization/concepts.h"

namespace portal
{
class ArchiveObject;

template <typename T>
concept Archiveable = requires(T t, ArchiveObject& s) {
    { t.archive(s) } -> std::same_as<void>;
};

template <typename T>
concept Dearchiveable = requires(T t, ArchiveObject& d) {
    { T::dearchive(d) } -> std::same_as<T>;
};

class ArchiveObject
{
public:
    using PropertyName = std::string_view;
    virtual ~ArchiveObject() = default;

    template <typename T> requires (std::integral<T> || std::floating_point<T>) && (!std::is_same_v<T, bool>)
    void add_property(const PropertyName name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                serialize::get_property_type<T>(),
                reflection::PropertyContainerType::scalar,
                1
            }
            );
    }

    template <serialize::Vector T>
    void add_property(const PropertyName name, const T& t)
    {
        if constexpr (Archiveable<typename T::value_type>)
        {
            Buffer buffer = Buffer::allocate(t.size() * sizeof(ArchiveObject));
            for (int i = 0; i < t.size(); ++i)
            {
                auto* object = new(buffer.as<ArchiveObject*>() + i) ArchiveObject();
                t[i].archive(*object);
            }

            add_property_to_map(
                name,
                {
                    std::move(buffer),
                    reflection::PropertyType::object,
                    reflection::PropertyContainerType::array,
                    t.size()
                }
                );
        }
        else
        {
            Buffer buffer = Buffer::allocate(t.size() * sizeof(ArchiveObject));
            for (size_t i = 0; i < t.size(); ++i)
            {
                auto* object = new(buffer.as<ArchiveObject*>() + i) ArchiveObject();
                object->add_property("v", t[i]);
            }

            constexpr auto property_type = (serialize::String<typename T::value_type>)
                ? reflection::PropertyType::null_term_string
                : serialize::get_property_type<typename T::value_type>();

            add_property_to_map(
                name,
                {
                    std::move(buffer),
                    property_type,
                    reflection::PropertyContainerType::array,
                    t.size()
                }
                );
        }
    }

    template <serialize::String T>
    void add_property(const PropertyName name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::copy(t.data(), t.size() + 1),
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                t.size() + 1
            }
            );
    }

    template <serialize::GlmVec1 T>
    void add_property(const PropertyName name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vec1,
                1
            }
            );
    }

    template <serialize::GlmVec2 T>
    void add_property(const PropertyName name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vec2,
                2
            }
            );
    }

    template <serialize::GlmVec3 T>
    void add_property(const PropertyName name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vec3,
                3
            }
            );
    }

    template <serialize::GlmVec4 T>
    void add_property(const PropertyName name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vec4,
                4
            }
            );
    }

    template <serialize::Map T> requires std::is_convertible_v<typename T::key_type, PropertyName>
    void add_property(const PropertyName name, const T& t)
    {
        auto* child = create_child(name);
        for (auto& [key, value] : t)
        {
            child->add_property(key, value);
        }
    }

    template <typename T>
    void add_property(const PropertyName, const T&)
    {
        static_assert(false, "Not implemented");
    }

    template <>
    void add_property<bool>(const PropertyName name, const bool& b)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<bool>(b),
                reflection::PropertyType::boolean,
                reflection::PropertyContainerType::scalar,
                1
            }
            );
    }


    template <>
    void add_property<uint128_t>(const PropertyName name, const uint128_t& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<uint128_t>(t),
                reflection::PropertyType::integer128,
                reflection::PropertyContainerType::scalar,
                1
            }
            );
    }

    void add_binary_block(const PropertyName name, const std::vector<std::byte>& data)
    {
        add_binary_block(name, {data.data(), data.size()});
    }

    void add_binary_block(const PropertyName name, const Buffer& buffer)
    {
        add_property_to_map(
            name,
            {
                Buffer::copy(buffer),
                reflection::PropertyType::binary,
                reflection::PropertyContainerType::array,
                buffer.size
            }
            );
    }

    template <Archiveable T>
    void add_property(const PropertyName name, const T& t)
    {
        auto* child = create_child(name);
        t.archive(*child);
    }

    template <typename T>
    bool get_property(const PropertyName name, T& t);

    template <>
    bool get_property<bool>(const PropertyName name, bool& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;

        out = *property.value.as<bool*>();
        return true;
    }

    template <typename T> requires (std::integral<T>) && (!std::is_same_v<T, bool>)
    bool get_property(const PropertyName name, T& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;

        out = *property.value.as<T*>();
        return true;
    }

    template <typename T> requires std::floating_point<T>
    bool get_property(const PropertyName name, T& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;

        if (property.type != serialize::get_property_type<T>())
        {
            if constexpr (std::is_same_v<T, float>)
            {
                out = static_cast<T>(*property.value.as<double*>());
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                out = static_cast<T>(*property.value.as<float*>());
            }
            else
            {
                LOG_ERROR_TAG("Serialization", "Property {} type mismatch", name);
                return false;
            }
        }
        else
        {
            out = *property.value.as<T*>();
        }
        return true;
    }

    template <>
    bool get_property<uint128_t>(const PropertyName name, uint128_t& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;

        out = *property.value.as<uint128_t*>();
        return true;
    }

    template <serialize::Vector T>
    bool get_property(PropertyName name, T& out)
    {
        using ValueType = typename T::value_type;
        out.clear();
        const auto& [value, type, container_type, elements_number] = property_map[std::string(name)];
        if (container_type == reflection::PropertyContainerType::invalid)
            return false;

        PORTAL_ASSERT(container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        if constexpr (Archiveable<ValueType>)
        {
            auto* objects = value.as<ArchiveObject*>();
            for (size_t i = 0; i < elements_number; ++i)
            {
                out.push_back(ValueType::dearchive(objects[i]));
            }
        }
        else
        {
            auto* objects = value.as<ArchiveObject*>();
            for (size_t i = 0; i < elements_number; ++i)
            {
                ValueType v;
                if (!objects[i].get_property("v", v))
                {
                    LOG_ERROR_TAG("Serialization", "Failed to get property from ArchiveObject {}", name);
                    return false;
                }
                out.push_back(static_cast<ValueType>(v));
            }
        }
        return true;
    }

    template <serialize::String T>
    bool get_property(PropertyName name, T& out)
    {
        const auto& [value, type, container_type, elements_number] = property_map[std::string(name)];
        if (type == reflection::PropertyType::invalid)
            return false;

        size_t string_length;
        if (container_type == reflection::PropertyContainerType::null_term_string)
            string_length = elements_number - 1;
        else if (container_type == reflection::PropertyContainerType::string)
            string_length = elements_number;
        else
        {
            LOG_ERROR_TAG("Serialization", "Property {} container type mismatch", name);
            string_length = 0;
        }

        const auto* data = value.as<const typename T::value_type*>();
        out = T(data, string_length);

        return true;
    }

    template <serialize::GlmVec1 T>
    bool get_property(PropertyName name, T& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vec1, "Property {} container type mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <serialize::GlmVec2 T>
    bool get_property(PropertyName name, T& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vec2, "Property {} container type mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <serialize::GlmVec3 T>
    bool get_property(PropertyName name, T& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vec3, "Property {} container type mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <serialize::GlmVec4 T>
    bool get_property(PropertyName name, T& out)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vec4, "Property {} container type mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <serialize::Map T> requires std::is_convertible_v<typename T::key_type, PropertyName>
    bool get_property(const PropertyName name, T& out)
    {
        using ValueType = typename T::mapped_type;
        out.clear();
        auto* child = get_object(name);
        if (!child)
            return false;

        for (const auto& [key, property] : child->property_map)
        {
            if constexpr (Dearchiveable<typename T::mapped_type>)
            {
                auto* value_child = child->get_object(key);
                if (value_child)
                {
                    out[key] = ValueType::dearchive(*value_child);
                }
            }
            else
            {
                out[key] = *property.value.as<typename T::mapped_type*>();
            }
        }
        return true;
    }

    template <Archiveable T>
    bool get_property(const PropertyName name, T& out)
    {
        auto* child = get_object(name);
        if (!child)
            return false;

        out = T::dearchive(*child);
        return true;
    }

    bool get_binary_block(const PropertyName name, Buffer& buffer)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.type == reflection::PropertyType::binary, "Property {} type mismatch", name);
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        buffer = Buffer::copy(property.value);
        return true;
    }

    bool get_binary_block(const PropertyName name, std::vector<std::byte>& data)
    {
        const auto& property = property_map[std::string(name)];
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.type == reflection::PropertyType::binary, "Property {} type mismatch", name);
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        data.assign(property.value.as<const std::byte*>(), property.value.as<const std::byte*>() + property.value.size);
        return true;
    }

    virtual ArchiveObject* create_child(PropertyName name);
    virtual ArchiveObject* get_object(PropertyName name);

protected:
    virtual reflection::Property& add_property_to_map(PropertyName name, reflection::Property&& property);
    std::unordered_map<std::string, reflection::Property> property_map;

    friend class JsonArchive;
};


} // namespace portal
