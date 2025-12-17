//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <concepts>
#include <filesystem>
#include <string>
#include <variant>

#include <llvm/ADT/StringMap.h>
#include <portal/core/buffer.h>
#include <portal/core/reflection/property.h>
#include <portal/core/reflection/property_concepts.h>

#include "portal/serialization/archive.h"


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

    ArchiveObject() = default;
    ArchiveObject(const ArchiveObject& other) = default;
    ArchiveObject(ArchiveObject&& other) noexcept = default;
    ArchiveObject& operator=(const ArchiveObject& other) = default;
    ArchiveObject& operator=(ArchiveObject&& other) = default;

    void update(const ArchiveObject& other);

    template <typename T> requires (std::integral<T> || std::floating_point<T>) && (!std::is_same_v<T, bool>)
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                reflection::get_property_type<T>(),
                reflection::PropertyContainerType::scalar,
                1
            }
        );
    }

    template <reflection::SmallVector T>
    void add_property(const PropertyName& name, const T& t)
    {
        if constexpr (Archiveable<typename T::ValueParamT>)
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

            constexpr auto property_type = (reflection::String<typename T::ValueParamT>)
                                               ? reflection::PropertyType::null_term_string
                                               : reflection::get_property_type<typename T::ValueParamT>();

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

    template <reflection::Vector T>
    void add_property(const PropertyName& name, const T& t)
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

            constexpr auto property_type = (reflection::String<typename T::value_type>)
                                               ? reflection::PropertyType::null_term_string
                                               : reflection::get_property_type<typename T::value_type>();

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

    template <reflection::String T>
    void add_property(const PropertyName& name, const T& t)
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

    template <reflection::GlmVec1 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                1
            }
        );
    }

    template <reflection::GlmVec2 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                2
            }
        );
    }

    template <reflection::GlmVec3 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                3
            }
        );
    }

    template <reflection::GlmVec4 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name,
            {
                Buffer::create<T>(t),
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                4
            }
        );
    }

    template <reflection::Map T> requires std::is_convertible_v<typename T::key_type, PropertyName>
    void add_property(const PropertyName& name, const T& t)
    {
        auto* child = create_child(name);
        for (auto& [key, value] : t)
        {
            child->add_property(key, value);
        }
    }

    template <typename T>
    void add_property(const PropertyName&, const T&) = delete;

    void add_property(const PropertyName& name, const char* t);
    void add_property(const PropertyName& name, const std::filesystem::path& t);

    void add_binary_block(const PropertyName& name, const std::vector<std::byte>& data)
    {
        add_binary_block(name, {data.data(), data.size()});
    }

    void add_binary_block(const PropertyName& name, const Buffer& buffer)
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
    void add_property(const PropertyName& name, const T& t)
    {
        auto* child = create_child(name);
        t.archive(*child);
    }

    template <typename T>
    bool get_property(const PropertyName& name, T& t) = delete;

    template <typename T> requires (std::integral<T>) && (!std::is_same_v<T, bool>)
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;

        out = *property.value.as<T*>();
        return true;
    }

    template <typename T> requires std::floating_point<T>
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;

        if (property.type != reflection::get_property_type<T>())
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

    template <reflection::Vector T>
    bool get_property(const PropertyName& name, T& out)
    {
        out.clear();
        const auto& prop = get_property_from_map(name);
        if (prop.container_type == reflection::PropertyContainerType::invalid)
            return false;

        PORTAL_ASSERT(prop.container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        return format_array<T, typename T::value_type>(name, prop, out);
    }

    template <reflection::SmallVector T>
    bool get_property(const PropertyName& name, T& out)
    {
        out.clear();
        const auto& prop = get_property_from_map(name);
        if (prop.container_type == reflection::PropertyContainerType::invalid)
            return false;

        PORTAL_ASSERT(prop.container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        return format_array<T, typename T::ValueParamT>(name, prop, out);
    }

    template <reflection::String T>
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& [value, type, container_type, elements_number] = get_property_from_map(name);
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

    template <reflection::GlmVec1 T>
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property {} container type mismatch", name);
        PORTAL_ASSERT(property.elements_number == 1, "Property {} elements number mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <reflection::GlmVec2 T>
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property {} container type mismatch", name);
        PORTAL_ASSERT(property.elements_number == 2, "Property {} elements number mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <reflection::GlmVec3 T>
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property {} container type mismatch", name);
        PORTAL_ASSERT(property.elements_number == 3, "Property {} elements number mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <reflection::GlmVec4 T>
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property {} container type mismatch", name);
        PORTAL_ASSERT(property.elements_number == 4, "Property {} elements number mismatch", name);

        out = T(*property.value.as<T*>());
        return true;
    }

    template <reflection::Map T> requires std::is_convertible_v<typename T::key_type, PropertyName>
    bool get_property(const PropertyName& name, T& out)
    {
        using ValueType = T::mapped_type;
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
                    out[std::string(key)] = ValueType::dearchive(*value_child);
                }
            }
            else
            {
                out[std::string(key)] = *property.value.as<typename T::mapped_type*>();
            }
        }
        return true;
    }

    template <Archiveable T>
    bool get_property(const PropertyName& name, T& out)
    {
        auto* child = get_object(name);
        if (!child)
            return false;

        out = T::dearchive(*child);
        return true;
    }

    bool get_property(const PropertyName& name, std::filesystem::path& out);

    bool get_binary_block(const PropertyName& name, Buffer& buffer)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.type == reflection::PropertyType::binary, "Property {} type mismatch", name);
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        buffer = Buffer::copy(property.value);
        return true;
    }

    bool get_binary_block(const PropertyName& name, std::vector<std::byte>& data)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;
        PORTAL_ASSERT(property.type == reflection::PropertyType::binary, "Property {} type mismatch", name);
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::array, "Property {} container type mismatch", name);

        data.assign(property.value.as<const std::byte*>(), property.value.as<const std::byte*>() + property.value.size);
        return true;
    }

    virtual ArchiveObject* create_child(PropertyName name);
    virtual ArchiveObject* get_object(PropertyName name);

    // Iterator support for range-based for loops
    auto begin() { return property_map.begin(); }
    auto end() { return property_map.end(); }
    auto begin() const { return property_map.begin(); }
    auto end() const { return property_map.end(); }

protected:
    template <typename T, typename ValueType> requires (reflection::Vector<T> || reflection::SmallVector<T>)
    bool format_array(const PropertyName& name, const reflection::Property& prop, T& out) const
    {
        const auto& [value, type, container_type, elements_number] = prop;

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

    virtual reflection::Property& get_property_from_map(PropertyName name);
    virtual reflection::Property& add_property_to_map(PropertyName name, reflection::Property&& property);

#ifdef PORTAL_DEBUG
    std::unordered_map<std::string, reflection::Property> property_map;
#else
    llvm::StringMap<reflection::Property> property_map;
#endif

    friend class JsonArchive;
};

template <>
inline void ArchiveObject::add_property<bool>(const PropertyName& name, const bool& b)
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
inline void ArchiveObject::add_property<uint128_t>(const PropertyName& name, const uint128_t& t)
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

template <>
inline bool ArchiveObject::get_property<bool>(const PropertyName& name, bool& out)
{
    const auto& property = get_property_from_map(name);
    if (property.type == reflection::PropertyType::invalid)
        return false;

    out = *property.value.as<bool*>();
    return true;
}

template <>
inline bool ArchiveObject::get_property<uint128_t>(const PropertyName& name, uint128_t& out)
{
    const auto& property = get_property_from_map(name);
    if (property.type == reflection::PropertyType::invalid)
        return false;

    out = *property.value.as<uint128_t*>();
    return true;
}
} // namespace portal
