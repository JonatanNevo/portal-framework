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
#include <glaze/core/reflect.hpp>
#include <portal/core/buffer.h>
#include <portal/core/reflection/property.h>
#include <portal/core/reflection/property_concepts.h>

#include "portal/core/strings/string_id.h"
#include "portal/core/strings/string_utils.h"
#include "portal/serialization/archive.h"


namespace portal
{
class ArchiveObject;

/**
 * @brief Non-intrusive archiving customization point.
 *
 * Specialize this template to enable archiving for types you don't control.
 * The specialization must provide static archive() and dearchive() functions.
 *
 * Example:
 * @code
 * // For a third-party type you can't modify:
 * template <>
 * struct Archivable<ThirdPartyConfig> {
 *     static void archive(const ThirdPartyConfig& obj, ArchiveObject& ar) {
 *         ar.add_property("name", obj.getName());
 *         ar.add_property("value", obj.getValue());
 *     }
 *     static ThirdPartyConfig dearchive(ArchiveObject& ar) {
 *         std::string name;
 *         int value;
 *         ar.get_property("name", name);
 *         ar.get_property("value", value);
 *         return ThirdPartyConfig(name, value);
 *     }
 * };
 * @endcode
 *
 * @tparam T The type to provide archiving support for
 */
template <typename T>
struct Archivable;

/**
 * @brief Concept for types with an archive() method that writes named properties to an ArchiveObject.
 *
 * Example:
 * @code
 * struct Config {
 *     void archive(ArchiveObject& ar) const {
 *         ar.add_property("name", name);
 *         ar.add_property("value", value);
 *     }
 * };
 * @endcode
 */
template <typename T>
concept ArchiveableConcept = requires(T t, ArchiveObject& s) {
    { t.archive(s) } -> std::same_as<void>;
};

/**
 * @brief Concept for types with a static dearchive() method that reads named properties from an ArchiveObject.
 *
 * Example:
 * @code
 * struct Config {
 *     static Config dearchive(ArchiveObject& ar) {
 *         Config c;
 *         ar.get_property("name", c.name);
 *         ar.get_property("value", c.value);
 *         return c;
 *     }
 * };
 * @endcode
 */
template <typename T>
concept DearchiveableConcept = requires(T t, ArchiveObject& d) {
    { T::dearchive(d) } -> std::same_as<T>;
};

/**
 * @brief Concept for types with an Archivable<T> specialization providing archive().
 *
 * This enables non-intrusive archiving for types you don't control.
 *
 * Example:
 * @code
 * template <>
 * struct Archivable<ExternalType> {
 *     static void archive(const ExternalType& obj, ArchiveObject& ar) {
 *         ar.add_property("field", obj.field);
 *     }
 * };
 * @endcode
 */
template <typename T>
concept ExternalArchiveableConcept = requires(const T& t, ArchiveObject& ar) {
    { Archivable<std::remove_cvref_t<T>>::archive(t, ar) } -> std::same_as<void>;
};

/**
 * @brief Concept for types with an Archivable<T> specialization providing dearchive().
 *
 * This enables non-intrusive dearchiving for types you don't control.
 *
 * Example:
 * @code
 * template <>
 * struct Archivable<ExternalType> {
 *     static ExternalType dearchive(ArchiveObject& ar) {
 *         ExternalType obj;
 *         ar.get_property("field", obj.field);
 *         return obj;
 *     }
 * };
 * @endcode
 */
template <typename T>
concept ExternalDearchiveableConcept = requires(ArchiveObject& ar) {
    { Archivable<std::remove_cvref_t<T>>::dearchive(ar) } -> std::same_as<T>;
};

/**
 * @brief Format-agnostic named-property serialization using the visitor pattern.
 *
 * Intermediate object graph that can be serialized to JSON, XML, binary, etc. Uses named
 * properties instead of ordered streams (see Serializer for stream-based alternative).
 *
 * **Use for**: Config files, save games, human-readable formats, flexible schemas
 * **Avoid for**: Network packets, performance-critical paths (use Serializer instead)
 *
 * **Supported types**: Scalars, strings, GLM vectors, std::vector, std::map, enums (as strings),
 * custom types, binary data
 *
 * @code
 * struct Config {
 *     void archive(ArchiveObject& ar) const {
 *         ar.add_property("name", name);
 *         ar.add_property("value", value);
 *     }
 *     static Config dearchive(ArchiveObject& ar) {
 *         Config c;
 *         ar.get_property("name", c.name);
 *         ar.get_property("value", c.value);
 *         return c;
 *     }
 * };
 *
 * JsonArchive archive;
 * config.archive(archive);
 * archive.dump("config.json");
 * @endcode
 */
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

    /**
     * @brief Merges properties from another ArchiveObject into this one.
     *
     * Properties from the other object are copied into this object's property map.
     * If a property with the same name already exists, it will be replaced.
     *
     * @param other The source ArchiveObject containing properties to merge.
     */
    void update(const ArchiveObject& other);

    /**
     * @brief Adds a scalar numeric property (integer or floating-point) to the archive.
     *
     * Stores a copy of the scalar value with appropriate type metadata. Supports all
     * integral types (int8_t through int64_t, unsigned variants) and floating-point
     * types (float, double). Boolean values use a specialized template (see add_property<bool>).
     *
     * @tparam T Numeric type (integral or floating-point, excluding bool)
     * @param name Property name identifier for serialization/deserialization
     * @param t The scalar value to serialize
     */
    template <typename T>
        requires(std::integral<T> || std::floating_point<T>) && (!std::is_same_v<T, bool>)
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(name, {Buffer::create<T>(t), reflection::get_property_type<T>(), reflection::PropertyContainerType::scalar, 1});
    }

    /**
     * @brief Adds an llvm::SmallVector property to the archive.
     *
     * Handles two cases:
     * - If elements are Archiveable: Each element archives itself into a child ArchiveObject
     * - Otherwise: Wraps each element in an ArchiveObject with property name "v" (maintains type uniformity)
     *
     * @tparam T SmallVector type satisfying reflection::SmallVector concept
     * @param name Property name identifier
     * @param t The SmallVector to serialize
     */
    template <reflection::SmallVector T>
    void add_property(const PropertyName& name, const T& t)
    {
        using ValueT = typename T::ValueParamT;
        if constexpr (ArchiveableConcept<ValueT> || ExternalArchiveableConcept<ValueT>)
        {
            Buffer buffer = Buffer::allocate(t.size() * sizeof(ArchiveObject));
            for (size_t i = 0; i < t.size(); ++i)
            {
                auto* object = new (buffer.as<ArchiveObject*>() + i) ArchiveObject();
                if constexpr (ArchiveableConcept<ValueT>)
                    t[i].archive(*object);
                else
                    Archivable<ValueT>::archive(t[i], *object);
            }

            add_property_to_map(name, {std::move(buffer), reflection::PropertyType::object, reflection::PropertyContainerType::array, t.size()});
        }
        else
        {
            Buffer buffer = Buffer::allocate(t.size() * sizeof(ArchiveObject));
            for (size_t i = 0; i < t.size(); ++i)
            {
                auto* object = new (buffer.as<ArchiveObject*>() + i) ArchiveObject();
                object->add_property("v", t[i]);
            }

            constexpr auto property_type = (reflection::String<ValueT>) ? reflection::PropertyType::null_term_string
                                                                        : reflection::get_property_type<ValueT>();

            add_property_to_map(name, {std::move(buffer), property_type, reflection::PropertyContainerType::array, t.size()});
        }
    }

    /**
     * @brief Adds a std::vector property to the archive.
     *
     * Handles two cases:
     * - If elements are Archiveable: Each element archives itself into a child ArchiveObject
     * - Otherwise: Wraps each element in an ArchiveObject with property name "v" (maintains type uniformity)
     *
     * This dual approach enables serialization of both primitive types (int, float, string) and
     * complex user-defined types in a consistent manner.
     *
     * @tparam T Vector type satisfying reflection::Vector concept (std::vector<...>)
     * @param name Property name identifier
     * @param t The vector to serialize
     */
    template <reflection::Vector T>
    void add_property(const PropertyName& name, const T& t)
    {
        using ValueT = typename T::value_type;
        if constexpr (ArchiveableConcept<ValueT> || ExternalArchiveableConcept<ValueT>)
        {
            Buffer buffer = Buffer::allocate(t.size() * sizeof(ArchiveObject));
            for (size_t i = 0; i < t.size(); ++i)
            {
                auto* object = new (buffer.as<ArchiveObject*>() + i) ArchiveObject();
                if constexpr (ArchiveableConcept<ValueT>)
                    t[i].archive(*object);
                else
                    Archivable<ValueT>::archive(t[i], *object);
            }

            add_property_to_map(name, {std::move(buffer), reflection::PropertyType::object, reflection::PropertyContainerType::array, t.size()});
        }
        else
        {
            Buffer buffer = Buffer::allocate(t.size() * sizeof(ArchiveObject));
            for (size_t i = 0; i < t.size(); ++i)
            {
                auto* object = new (buffer.as<ArchiveObject*>() + i) ArchiveObject();
                object->add_property("v", t[i]);
            }

            constexpr auto property_type = (reflection::String<ValueT>) ? reflection::PropertyType::null_term_string
                                                                        : reflection::get_property_type<ValueT>();

            add_property_to_map(name, {std::move(buffer), property_type, reflection::PropertyContainerType::array, t.size()});
        }
    }

    /**
     * @brief Adds a string property to the archive.
     *
     * Handles std::string, std::string_view, and other types satisfying reflection::String concept.
     * The string is copied with null terminator for safe deserialization.
     *
     * @tparam T String type satisfying reflection::String concept
     * @param name Property name identifier
     * @param t The string to serialize
     */
    template <reflection::String T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name,
            {Buffer::copy(t.data(), t.size() + 1),
             reflection::PropertyType::character,
             reflection::PropertyContainerType::null_term_string,
             t.size() + 1});
    }

    /**
     * @brief Adds a GLM vec1 property (single-component vector).
     *
     * @tparam T GLM vec1 type (e.g., glm::vec1, glm::ivec1)
     * @param name Property name identifier
     * @param t The vector to serialize
     */
    template <reflection::GlmVec1 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name, {Buffer::create<T>(t), reflection::get_property_type<typename T::value_type>(), reflection::PropertyContainerType::vector, 1});
    }

    /**
     * @brief Adds a GLM vec2 property (two-component vector).
     *
     * @tparam T GLM vec2 type (e.g., glm::vec2, glm::ivec2, glm::dvec2)
     * @param name Property name identifier
     * @param t The vector to serialize
     */
    template <reflection::GlmVec2 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name, {Buffer::create<T>(t), reflection::get_property_type<typename T::value_type>(), reflection::PropertyContainerType::vector, 2});
    }

    /**
     * @brief Adds a GLM vec3 property (three-component vector).
     *
     * @tparam T GLM vec3 type (e.g., glm::vec3, glm::ivec3, glm::dvec3)
     * @param name Property name identifier
     * @param t The vector to serialize
     */
    template <reflection::GlmVec3 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name, {Buffer::create<T>(t), reflection::get_property_type<typename T::value_type>(), reflection::PropertyContainerType::vector, 3});
    }

    /**
     * @brief Adds a GLM vec4 property (four-component vector).
     *
     * @tparam T GLM vec4 type (e.g., glm::vec4, glm::ivec4, glm::dvec4)
     * @param name Property name identifier
     * @param t The vector to serialize
     */
    template <reflection::GlmVec4 T>
    void add_property(const PropertyName& name, const T& t)
    {
        add_property_to_map(
            name, {Buffer::create<T>(t), reflection::get_property_type<typename T::value_type>(), reflection::PropertyContainerType::vector, 4});
    }

    /**
     * @brief Adds a map property with string-convertible keys.
     *
     * Creates a child ArchiveObject and populates it with key-value pairs from the map.
     * Keys must be convertible to std::string_view. Values are serialized recursively.
     *
     * @tparam T Map type (std::unordered_map, std::map, etc.) with string-like keys
     * @param name Property name identifier
     * @param t The map to serialize
     */
    template <reflection::Map T>
        requires std::is_convertible_v<typename T::key_type, PropertyName>
    void add_property(const PropertyName& name, const T& t)
    {
        auto* child = create_child(name);
        for (auto& [key, value] : t)
        {
            child->add_property(key, value);
        }
    }

    /**
     * @brief Adds an enum property serialized as a string.
     *
     * Uses portal::to_string() to convert the enum to its string representation,
     * making the serialized format human-readable. Deserialization uses portal::from_string().
     *
     * @tparam T Enum type
     * @param name Property name identifier
     * @param e The enum value to serialize
     */
    template <typename T>
        requires std::is_enum_v<T>
    void add_property(const PropertyName& name, const T& e)
    {
        add_property(name, portal::to_string(e));
    }

    /**
     * @brief Adds a C-string property.
     * @param name Property name identifier
     * @param t Null-terminated C-string
     */
    void add_property(const PropertyName& name, const char* t);

    /**
     * @brief Adds a filesystem path property.
     * @param name Property name identifier
     * @param t Filesystem path
     */
    void add_property(const PropertyName& name, const std::filesystem::path& t);

    void add_property(const PropertyName& name, const StringId& string_id);

    /**
     * @brief Adds a binary data block property from a byte vector.
     *
     * Stores arbitrary binary data that doesn't fit standard types (images, compressed data, etc.).
     * The data is copied internally and marked with PropertyType::binary for special handling
     * during serialization (e.g., base64 encoding in JSON).
     *
     * @param name Property name identifier
     * @param data Binary data as a vector of bytes
     */
    void add_binary_block(const PropertyName& name, const std::vector<std::byte>& data) { add_binary_block(name, {data.data(), data.size()}); }

    /**
     * @brief Adds a binary data block property from a Buffer.
     *
     * @param name Property name identifier
     * @param buffer Buffer containing the binary data (will be copied)
     */
    void add_binary_block(const PropertyName& name, const Buffer& buffer)
    {
        add_property_to_map(name, {Buffer::copy(buffer), reflection::PropertyType::binary, reflection::PropertyContainerType::array, buffer.size});
    }

    /**
     * @brief Adds a custom Archiveable type as a nested property.
     *
     * Creates a child ArchiveObject and calls the type's archive() method to populate it.
     * This enables hierarchical serialization of user-defined types.
     *
     * @tparam T Type satisfying the Archiveable concept
     * @param name Property name identifier
     * @param t The object to serialize
     */
    template <ArchiveableConcept T>
    void add_property(const PropertyName& name, const T& t)
    {
        auto* child = create_child(name);
        t.archive(*child);
    }

    /**
     * @brief Adds a type with an Archivable<T> specialization as a nested property.
     *
     * Creates a child ArchiveObject and calls Archivable<T>::archive() to populate it.
     * This enables non-intrusive hierarchical serialization of external types.
     *
     * @tparam T Type satisfying the ExternalArchiveableConcept
     * @param name Property name identifier
     * @param t The object to serialize
     */
    template <ExternalArchiveableConcept T>
    void add_property(const PropertyName& name, const T& t)
    {
        auto* child = create_child(name);
        Archivable<std::remove_cvref_t<T>>::archive(t, *child);
    }

    /**
     * @brief Adds a property using glaze reflection for types with reflection metadata.
     *
     * Automatically serializes types that have glaze reflection metadata defined. Uses
     * compile-time reflection to extract field names and values, creating a nested
     * ArchiveObject with each field as a named property. Fallback overload for types
     * that don't implement archive() but have glaze metadata.
     *
     * @tparam T Type with glaze reflection metadata (glz::reflect)
     * @param name Property name identifier
     * @param t The object to serialize
     */
    template <typename T>
    void add_property(const PropertyName& name, const T& t)
    {
        constexpr auto N = glz::reflect<T>::size;
        if constexpr (N > 0)
        {
            auto* child = create_child(name);

            glz::for_each<N>(
                [&]<size_t I>()
                {
                    child->add_property(glz::reflect<T>::keys[I], glz::get_member(t, glz::get<I>(glz::to_tie(t))));
                }
            );
        }
    }

    /**
     * @brief Retrieves an enum property deserialized from its string representation.
     *
     * Reads the property as a string and converts it back to the enum using portal::from_string().
     *
     * @tparam T Enum type
     * @param name Property name identifier
     * @param out Output parameter to store the enum value
     * @return true if property exists and conversion succeeds, false otherwise
     */
    template <typename T>
        requires std::is_enum_v<T>
    bool get_property(const PropertyName& name, T& out)
    {
        std::string out_string;
        if (!get_property<std::string>(name, out_string))
            return false;

        out = portal::from_string<T>(out_string);
        return true;
    }

    /**
     * @brief Retrieves an integral property value.
     *
     * Reads a scalar integer value (int8_t through int64_t, unsigned variants).
     *
     * @tparam T Integral type (excluding bool)
     * @param name Property name identifier
     * @param out Output parameter to store the value
     * @return true if property exists and type matches, false otherwise
     */
    template <typename T>
        requires(std::integral<T>) && (!std::is_same_v<T, bool>)
    bool get_property(const PropertyName& name, T& out)
    {
        const auto& property = get_property_from_map(name);
        if (property.type == reflection::PropertyType::invalid)
            return false;

        out = *property.value.as<T*>();
        return true;
    }

    /**
     * @brief Retrieves a floating-point property value with automatic float/double conversion.
     *
     * Handles JSON's ambiguity between float and double by automatically converting
     * between the two when necessary.
     *
     * @tparam T Floating-point type (float or double)
     * @param name Property name identifier
     * @param out Output parameter to store the value
     * @return true if property exists and is numeric, false otherwise
     */
    template <typename T>
        requires std::floating_point<T>
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

    /**
     * @brief Retrieves a std::vector property.
     *
     * Clears the output vector and populates it with elements from the archived array.
     * Handles both primitive element types and Archiveable custom types.
     *
     * @tparam T Vector type satisfying reflection::Vector concept
     * @param name Property name identifier
     * @param out Output vector to populate
     * @return true if property exists and is an array, false otherwise
     */
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

    /**
     * @brief Retrieves an llvm::SmallVector property.
     *
     * Clears the output SmallVector and populates it with elements from the archived array.
     *
     * @tparam T SmallVector type satisfying reflection::SmallVector concept
     * @param name Property name identifier
     * @param out Output SmallVector to populate
     * @return true if property exists and is an array, false otherwise
     */
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

    /**
     * @brief Retrieves a string property.
     *
     * Handles both null-terminated and length-prefixed string storage.
     *
     * @tparam T String type satisfying reflection::String concept
     * @param name Property name identifier
     * @param out Output string to populate
     * @return true if property exists and is a string, false otherwise
     */
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

    /**
     * @brief Retrieves a GLM vec1 property (single-component vector).
     *
     * @tparam T GLM vec1 type (e.g., glm::vec1, glm::ivec1)
     * @param name Property name identifier
     * @param out Output parameter to store the vector
     * @return true if property exists and is a 1-component vector, false otherwise
     */
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

    /**
     * @brief Retrieves a GLM vec2 property (two-component vector).
     *
     * @tparam T GLM vec2 type (e.g., glm::vec2, glm::ivec2, glm::dvec2)
     * @param name Property name identifier
     * @param out Output parameter to store the vector
     * @return true if property exists and is a 2-component vector, false otherwise
     */
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

    /**
     * @brief Retrieves a GLM vec3 property (three-component vector).
     *
     * @tparam T GLM vec3 type (e.g., glm::vec3, glm::ivec3, glm::dvec3)
     * @param name Property name identifier
     * @param out Output parameter to store the vector
     * @return true if property exists and is a 3-component vector, false otherwise
     */
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

    /**
     * @brief Retrieves a GLM vec4 property (four-component vector).
     *
     * @tparam T GLM vec4 type (e.g., glm::vec4, glm::ivec4, glm::dvec4)
     * @param name Property name identifier
     * @param out Output parameter to store the vector
     * @return true if property exists and is a 4-component vector, false otherwise
     */
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

    /**
     * @brief Retrieves a map property with string-convertible keys.
     *
     * Clears the output map and populates it with key-value pairs from the child ArchiveObject.
     * Handles both primitive value types and Dearchiveable custom types.
     *
     * @tparam T Map type (std::unordered_map, std::map, etc.) with string-like keys
     * @param name Property name identifier
     * @param out Output map to populate
     * @return true if property exists and is an object, false otherwise
     */
    template <reflection::Map T>
        requires std::is_convertible_v<typename T::key_type, PropertyName>
    bool get_property(const PropertyName& name, T& out)
    {
        using ValueType = T::mapped_type;
        out.clear();
        auto* child = get_object(name);
        if (!child)
            return false;

        for (const auto& [key, property] : child->property_map)
        {
            if constexpr (DearchiveableConcept<typename T::mapped_type>)
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

    /**
     * @brief Retrieves a custom Archiveable type from a nested property.
     *
     * Gets the child ArchiveObject and calls the type's static dearchive() method to reconstruct it.
     *
     * @tparam T Type satisfying the Archiveable concept
     * @param name Property name identifier
     * @param out Output parameter to store the dearchived object
     * @return true if property exists and is an object, false otherwise
     */
    template <ArchiveableConcept T>
    bool get_property(const PropertyName& name, T& out)
    {
        auto* child = get_object(name);
        if (!child)
            return false;

        out = T::dearchive(*child);
        return true;
    }

    /**
     * @brief Retrieves a type with an Archivable<T> specialization from a nested property.
     *
     * Gets the child ArchiveObject and calls Archivable<T>::dearchive() to reconstruct it.
     * This enables non-intrusive deserialization of external types.
     *
     * @tparam T Type satisfying the ExternalDearchiveableConcept
     * @param name Property name identifier
     * @param out Output parameter to store the dearchived object
     * @return true if property exists and is an object, false otherwise
     */
    template <ExternalDearchiveableConcept T>
    bool get_property(const PropertyName& name, T& out)
    {
        auto* child = get_object(name);
        if (!child)
            return false;

        out = Archivable<std::remove_cvref_t<T>>::dearchive(*child);
        return true;
    }

    /**
     * @brief Retrieves a filesystem path property.
     * @param name Property name identifier
     * @param out Output parameter to store the path
     * @return true if property exists, false otherwise
     */
    bool get_property(const PropertyName& name, std::filesystem::path& out);

    bool get_property(const PropertyName& name, StringId& out);

    /**
     * @brief Retrieves a binary data block property into a Buffer.
     *
     * @param name Property name identifier
     * @param buffer Output Buffer to receive a copy of the binary data
     * @return true if the property exists and is binary type, false otherwise
     */
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

    /**
     * @brief Retrieves a binary data block property into a byte vector.
     *
     * @param name Property name identifier
     * @param data Output vector to receive the binary data
     * @return true if the property exists and is binary type, false otherwise
     */
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

    /**
     * @brief Retrieves a property using glaze reflection for types with reflection metadata.
     *
     * Automatically deserializes types that have glaze reflection metadata defined. Uses
     * compile-time reflection to extract field names and populate each field from the nested
     * ArchiveObject. Fallback overload for types that don't implement dearchive() but have
     * glaze metadata.
     *
     * @tparam T Type with glaze reflection metadata (glz::reflect)
     * @param name Property name identifier
     * @param t Output parameter to populate with deserialized values
     * @return true if property exists and all fields were read, false otherwise
     */
    template <typename T>
    bool get_property(const PropertyName& name, T& t)
    {
        constexpr auto N = glz::reflect<T>::size;
        if constexpr (N > 0)
        {
            auto* child = get_object(name);
            if (child == nullptr)
                return false;

            glz::for_each<N>(
                [&]<size_t I>()
                {
                    child->get_property(glz::reflect<T>::keys[I], glz::get_member(t, glz::get<I>(glz::to_tie(t))));
                }
            );

            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * @brief Creates a new child ArchiveObject and adds it as a property.
     *
     * Creates a nested ArchiveObject for hierarchical serialization. Used internally
     * by add_property() when serializing Archiveable types or maps. The child becomes
     * owned by this ArchiveObject and is stored in the property map.
     *
     * @param name Property name for the child object
     * @return Pointer to the newly created child ArchiveObject (non-owning)
     */
    virtual ArchiveObject* create_child(PropertyName name);

    /**
     * @brief Retrieves a child ArchiveObject by name.
     *
     * Used during deserialization to access nested objects. Returns null if no child
     * with the given name exists.
     *
     * @param name Property name of the child object
     * @return Pointer to the child ArchiveObject, or nullptr if not found
     */
    virtual ArchiveObject* get_object(PropertyName name) const;

    // Iterator support for range-based for loops
    auto begin() { return property_map.begin(); }
    auto end() { return property_map.end(); }
    auto begin() const { return property_map.begin(); }
    auto end() const { return property_map.end(); }

protected:
    template <typename T, typename ValueType>
        requires(reflection::Vector<T> || reflection::SmallVector<T>)
    bool format_array(const PropertyName& name, const reflection::Property& prop, T& out) const
    {
        const auto& [value, type, container_type, elements_number] = prop;

        if constexpr (ArchiveableConcept<ValueType> || ExternalDearchiveableConcept<ValueType>)
        {
            auto* objects = value.as<ArchiveObject*>();
            for (size_t i = 0; i < elements_number; ++i)
            {
                if constexpr (ArchiveableConcept<ValueType>)
                    out.push_back(ValueType::dearchive(objects[i]));
                else
                    out.push_back(Archivable<ValueType>::dearchive(objects[i]));
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

    [[nodiscard]] virtual reflection::Property& get_property_from_map(PropertyName name);
    [[nodiscard]] virtual const reflection::Property& get_property_from_map(PropertyName name) const;
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
    add_property_to_map(name, {Buffer::create<bool>(b), reflection::PropertyType::boolean, reflection::PropertyContainerType::scalar, 1});
}

template <>
inline void ArchiveObject::add_property<uint128_t>(const PropertyName& name, const uint128_t& t)
{
    add_property_to_map(name, {Buffer::create<uint128_t>(t), reflection::PropertyType::integer128, reflection::PropertyContainerType::scalar, 1});
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
