//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <concepts>
#include <string>
#include <variant>

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Allocator.h"
#include "portal/core/buffer.h"
#include "portal/serialization/archive/property.h"

namespace portal
{
/*class Archiver;
class Dearchiver;

template <typename T>
concept Archiveable = requires(T t, Archiver& s) {
    { t.archive(s) } -> std::same_as<void>;
};

template <typename T>
concept Dearchiveable = requires(T t, Dearchiver& d) {
    { T::dearchive(d) } -> std::same_as<T>;
};

class Archiver
{
public:
    virtual ~Archiver() = default;
    virtual void archive() = 0;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void add_property(const std::string& name, T& t)
    {
        add_property(name, {Buffer{&t, sizeof(T)}, archiving::get_property_type<T>(), archiving::PropertyContainerType::scalar, 1});
    }

    template <typename T> requires std::is_same_v<T, uint128_t>
    void add_property(const std::string& name, T& t)
    {
        add_property(name, {Buffer{&t, sizeof(T)}, archiving::PropertyType::integer128, archiving::PropertyContainerType::scalar, 1});
    }

    template <archiving::Vector T> requires !Archiveable<typename T::value_type>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {Buffer{t.data(), t.size() * sizeof(typename T::value_type)},
             archiving::get_property_type<typename T::value_type>(),
             archiving::PropertyContainerType::array,
             t.size()}
            );
    }

    template <archiving::String T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {Buffer{t.data(), (t.size() * sizeof(typename T::value_type)) + 1},
             archiving::PropertyType::character,
             archiving::PropertyContainerType::null_term_string,
             t.size() + 1}
            );
    }

    template <archiving::GlmVec1 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {Buffer{&t.x, sizeof(typename T::value_type)},
             archiving::get_property_type<typename T::value_type>(),
             archiving::PropertyContainerType::vec1,
             1}
            );
    }

    template <archiving::GlmVec2 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {Buffer{&t.x, 2 * sizeof(typename T::value_type)},
             archiving::get_property_type<typename T::value_type>(),
             archiving::PropertyContainerType::vec2,
             2}
            );
    }

    template <archiving::GlmVec3 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {Buffer{&t.x, 3 * sizeof(typename T::value_type)},
             archiving::get_property_type<typename T::value_type>(),
             archiving::PropertyContainerType::vec3,
             3}
            );
    }

    template <archiving::GlmVec4 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {Buffer{&t.x, 4 * sizeof(typename T::value_type)},
             archiving::get_property_type<typename T::value_type>(),
             archiving::PropertyContainerType::vec4,
             4}
            );
    }

protected:
    virtual void add_property(const std::string& name, const archiving::Property& property) = 0;
};


class Dearchiver
{
public:
    virtual ~Dearchiver() = default;
    virtual void load() = 0;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.type == archiving::get_property_type<T>(), "Property {} type mismatch", name);

        out = *static_cast<T*>(property.value.data);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    template <typename T> requires std::is_same_v<T, uint128_t>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::PropertyType::integer128;
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.type == archiving::PropertyType::integer128, "Property {} type mismatch", name);

        out = *static_cast<T*>(property.value.data);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    template <archiving::Vector T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.container_type == archiving::PropertyContainerType::array, "Property {} container type mismatch", name);
        PORTAL_ASSERT(property.value.size / property.elements_number % sizeof(typename T::value_type) == 0, "Property {} size mismatch", name);

        auto array_length = property.elements_number;
        auto* data = static_cast<typename T::value_type*>(property.value.data);
        out = T(data, data + array_length);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    template <archiving::String T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        size_t string_length;
        if (property.container_type == archiving::PropertyContainerType::null_term_string)
            string_length = property.elements_number - 1;
        else if (property.container_type == archiving::PropertyContainerType::string)
            string_length = property.elements_number;
        else
        {
            LOG_ERROR_TAG("Serialization", "Property {} container type mismatch", name);
            string_length = 0;
        }

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data, string_length);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    template <archiving::GlmVec1 T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.container_type == archiving::PropertyContainerType::vec1, "Property {} container type mismatch", name);

        out = T(*static_cast<typename T::value_type*>(property.value.data));
        return true;
    }

    template <archiving::GlmVec2 T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.container_type == archiving::PropertyContainerType::vec2, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data[0], data[1]);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    template <archiving::GlmVec3 T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.container_type == archiving::PropertyContainerType::vec3, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data[0], data[1], data[2]);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    template <archiving::GlmVec4 T>
    bool get_property(const std::string& name, T& out)
    {
        archiving::Property property;
        property.type = archiving::get_property_type<T>();
        if (!get_property(name, property))
            return false;

        PORTAL_ASSERT(property.container_type == archiving::PropertyContainerType::vec4, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data[0], data[1], data[2], data[3]);

        if (property.value.allocated)
            property.value.release();

        return true;
    }

    // Copy override for all types
    template <typename T>
    T get_property(const std::string& name)
    {
        T out;
        get_property<T>(name, out);
        return out;
    }

protected:
    virtual bool get_property(const std::string& name, archiving::Property& out) = 0;
};*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ArchiveObject;
class ArchiveObjectIterator;
class NamedArchiveObject;
class ArchiveObjectConstIterator;

class Archivable
{
public:
    Archivable() = default;
    virtual ~Archivable() = default;
    virtual void load(const ArchiveObject* obj) = 0;
    virtual void save(ArchiveObject* obj) const = 0;
};

class TreeArchiveBase
{
    friend class ArchiveObject;

public:
    [[nodiscard]] float get_version() const { return version; }
    void set_version(const float v) { version = v; }

protected:
    float version = -1;
    llvm::BumpPtrAllocator allocator;
};

//**************************************************************************
//                ArchiveObject
//**************************************************************************

class ArchiveObject
{
    using PropertyName = const std::string_view;

public:
    ~ArchiveObject();
    ArchiveObject(const ArchiveObject&) = delete;

    [[nodiscard]] float get_version() const;

    void erase_all();

    void delete_property(PropertyName name);

    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    void set_property(PropertyName name, T n)
    {
        add_property_to_map(
            archiving::Property{
                .value = Buffer::copy(n),
                .type = archiving::get_property_type<T>(),
                .container_type = archiving::PropertyContainerType::scalar,
                .elements_number = 1
            },
            name
            );
    }

    void set_property(PropertyName name, bool b);

    void set_property(PropertyName name, const char* str)
    {
        const size_t length = strlen(str);
        add_property_to_map(
            archiving::Property{
                Buffer::copy(str, (length * sizeof(char)) + 1),
                archiving::PropertyType::character,
                archiving::PropertyContainerType::null_term_string,
                length + 1
            },
            name
            );
    }

    template <archiving::String T>
    void set_property(PropertyName name, const T& t)
    {
        add_property_to_map(
            archiving::Property{
                Buffer::copy(t.data(), (t.size() * sizeof(typename T::value_type)) + 1),
                archiving::PropertyType::character,
                archiving::PropertyContainerType::null_term_string,
                t.size() + 1
            },
            name
            );
    }

    template <size_t N>
    void set_property(PropertyName name, const llvm::SmallString<N>& str)
    {
        set_property(name, str.c_str());
    }

    template <archiving::GlmVec1 T>
    void set_property(PropertyName name, T& t)
    {
        add_property_to_map(
            archiving::Property{
                Buffer::copy(&t.x, sizeof(typename T::value_type)),
                archiving::get_property_type<typename T::value_type>(),
                archiving::PropertyContainerType::vec1,
                1
            },
            name
            );
    }

    template <archiving::GlmVec2 T>
    void set_property(PropertyName name, T& t)
    {
        add_property_to_map(
            archiving::Property{
                Buffer::copy(&t.x, 2 * sizeof(typename T::value_type)),
                archiving::get_property_type<typename T::value_type>(),
                archiving::PropertyContainerType::vec2,
                2
            },
            name
            );
    }

    template <archiving::GlmVec3 T>
    void set_property(PropertyName name, T& t)
    {
        add_property_to_map(
            archiving::Property{
                Buffer::copy(&t.x, 3 * sizeof(typename T::value_type)),
                archiving::get_property_type<typename T::value_type>(),
                archiving::PropertyContainerType::vec3,
                3
            },
            name
            );
    }

    template <archiving::GlmVec4 T>
    void set_property(PropertyName name, T& t)
    {
        add_property_to_map(
            archiving::Property{
                Buffer::copy(&t.x, 4 * sizeof(typename T::value_type)),
                archiving::get_property_type<typename T::value_type>(),
                archiving::PropertyContainerType::vec4,
                4
            },
            name
            );
    }

    template <archiving::Vector T>
    void set_property(PropertyName name, const T& v)
    {
        add_property_to_map(
            archiving::Property{
                Buffer::copy(v.data(), v.size() * sizeof(typename T::value_type)),
                archiving::get_property_type<typename T::value_type>(),
                archiving::PropertyContainerType::array,
                v.size()
            },
            name
            );
    }

    void set_property(PropertyName name, const std::vector<byte>& v)
    {
        llvm::SmallVector<byte> buffer;
        buffer.resize(v.size());
        for (int i = 0; i < v.size(); ++i)
        {
            buffer[i] = v[i];
        }
        set_binary_block(name, buffer);
    }

    template <archiving::Map T>
    void set_property(PropertyName name, const T& m)
    {
        ArchiveObject* object = create_child(name);
        int i = 0;
        for (auto& [key, value] : m)
        {
            object->set_property(fmt::format("k{}", i), key);
            object->set_property(fmt::format("v{}", i), value);
            i++;
        }
    }

    template <typename F, typename S>
    void set_property(PropertyName name, const std::pair<F, S>& p)
    {
        ArchiveObject* object_node = create_child(name);
        object_node->set_property("f", p.first);
        object_node->set_property("s", p.second);
    }

    void set_property(PropertyName name, const Archivable& t)
    {
        ArchiveObject* object_node = create_child(name);
        t.save(object_node);
    }

    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    bool get_property(PropertyName name, T& n)
    {
        auto* prop = find_property(name);
        if (!prop)
            return false;

        PORTAL_ASSERT(prop->container_type == archiving::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_ASSERT(prop->type == archiving::get_property_type<T>(), "Property type mismatch");
        PORTAL_ASSERT(prop->value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), prop->value.size);

        if (prop->type == archiving::get_property_type<T>())
        {
            n = *prop->value.as<T>();
            return true;
        }
        return false;
    }

    bool get_property(PropertyName name, bool& b);
    bool get_property(PropertyName name, std::string& str);

    template <size_t N>
    bool get_property(PropertyName name, llvm::SmallString<N>& str)
    {
        std::string a;
        const bool result = get_property(name, a);
        if (result)
            str = a;
        return result;
    }

    template <archiving::GlmVec1 T>
    bool get_property(PropertyName name, T& t)
    {

    }

    template <archiving::GlmVec2 T>
    bool get_property(PropertyName name, T& t)
    {}

    template <archiving::GlmVec3 T>
    bool get_property(PropertyName name, T& t)
    {}

    template <archiving::GlmVec4 T>
    bool get_property(PropertyName name, T& t)
    {}

    template <archiving::Vector T>
    bool get_property(PropertyName name, T& out) const
    {
        out.clear();
        const ArchiveObject* object = get_object(name);
        if (!object)
            return false;

        int i = 0;
        while (true)
        {
            typename T::value_type item;
            if (!object->get_property(fmt::format("i{}", i++), item))
                break;
            out.push_back(item);
        }
        return true;
    }

    bool get_property(PropertyName name, std::vector<portal::byte>& out) const
    {
        llvm::SmallString<64> buffer;
        if (!get_binary_block(name, buffer))
            return false;
        out.resize(buffer.size());
        for (int i = 0; i < buffer.size(); ++i)
        {
            out[i] = buffer[i];
        }
        return true;
    }

    template <archiving::Map T>
    bool get_property(PropertyName name, T& out) const
    {
        out.clear();
        const ArchiveObject* object = get_object(name);
        if (!object)
            return false;

        int i = 0;
        while (true)
        {
            typename T::key_type key;
            typename T::value_type item;
            if (!object->get_property(fmt::format("k{}", i), key))
                break;
            if (!object->get_property(fmt::format("v{}", i), item))



            )
            break;
            i++;
            out[key] = item;
        }
        return true;
    }

    template <typename F, typename S>
    bool get_property(PropertyName name, std::pair<F, S>& out) const
    {
        const ArchiveObject* object = get_object(name);
        if (!object)
            return false;

        if (!object->get_property("f", out.first))
            return false;

        if (!object->get_property("s", out.second))
            return false;

        return true;
    }

    bool get_property(PropertyName name, Archivable& t) const
    {
        const ArchiveObject* object_node = get_object(name);
        if (!object_node)
            return false;

        t.load(object_node);
        return true;
    }

    void set_binary_block(PropertyName name, const llvm::SmallVector<byte>& data);
    bool get_binary_block(PropertyName name, llvm::SmallVector<byte>& out) const;

    ArchiveObject* get_object(PropertyName name);
    const ArchiveObject* get_object(PropertyName name) const;

    ArchiveObject* create_child(PropertyName name);
    ArchiveObject* child(PropertyName name);

    void delete_object(PropertyName name);

    ArchiveObjectIterator get_first_object();
    ArchiveObjectIterator begin();
    ArchiveObjectIterator end();
    ArchiveObjectIterator begin() const;
    ArchiveObjectIterator end() const;

    struct PropertyDefinition
    {
        PropertyDefinition() = default;
        PropertyDefinition(const PropertyDefinition&) = default;

        PropertyDefinition(archiving::Property* value, PropertyName name): value(value), name(name) {}

        PropertyName name;
        archiving::Property* value = nullptr;
    };

    llvm::SmallVector<PropertyDefinition, 20> get_properties() const;

protected:
    ArchiveObject(TreeArchiveBase* target);

    archiving::Property& add_property_to_map(archiving::Property&& property, PropertyName name);
    archiving::Property& access_property_in_map(PropertyName name);

private:
    ArchiveObject(ArchiveObject&&) = default;

    const archiving::Property* find_property(PropertyName) const;
    static void erase_archive_object(ArchiveObject* object);

protected:
    llvm::StringMap<archiving::Property, llvm::BumpPtrAllocator&> property_map;
    TreeArchiveBase* paret_archive = nullptr;
};

} // namespace portal
