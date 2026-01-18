//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <glaze/core/reflect.hpp>

#include <portal/core/log.h>
#include <portal/core/reflection/property.h>

#include "portal/core/reflection/property_concepts.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
class Serializer;
class Deserializer;

/**
 * @brief Non-intrusive serialization customization point.
 *
 * Specialize this template to enable serialization for types you don't control.
 * The specialization must provide static serialize() and deserialize() functions.
 *
 * Example:
 * @code
 * // For a third-party type you can't modify:
 * template <>
 * struct SerializableType<ThirdPartyPacket> {
 *     static void serialize(const ThirdPartyPacket& obj, Serializer& s) {
 *         s.add_value(obj.getId());
 *         s.add_value(obj.getData());
 *     }
 *     static ThirdPartyPacket deserialize(Deserializer& d) {
 *         size_t id;
 *         std::string data;
 *         d.get_value(id);
 *         d.get_value(data);
 *         return ThirdPartyPacket(id, data);
 *     }
 * };
 * @endcode
 *
 * @tparam T The type to provide serialization support for
 */
template <typename T>
struct Serializable;

/**
 * @brief Concept for types with a serialize() method that writes data sequentially to a Serializer.
 *
 * Example:
 * @code
 * struct Packet {
 *     size_t sequence_id;
 *     std::string payload;
 *
 *     void serialize(Serializer& s) const {
 *         s.add_value(sequence_id);
 *         s.add_value(payload);
 *     }
 * };
 * @endcode
 */
template <typename T>
concept SerializableConcept = requires(const T t, Serializer& s) {
    { t.serialize(s) } -> std::same_as<void>;
};

/**
 * @brief Concept for types with a static deserialize() method that reads data from a Deserializer.
 *
 * Example:
 * @code
 * struct Packet {
 *     size_t sequence_id;
 *     std::string payload;
 *
 *     static Packet deserialize(Deserializer& d) {
 *         Packet p;
 *         d.get_value(p.sequence_id);
 *         d.get_value(p.payload);
 *         return p;
 *     }
 * };
 * @endcode
 */
template <typename T>
concept DeserializableConcept = requires(T t, Deserializer& d) {
    { T::deserialize(d) } -> std::same_as<T>;
};

/**
 * @brief Concept for types with a SerializableType<T> specialization providing serialize().
 *
 * This enables non-intrusive serialization for types you don't control.
 *
 * Example:
 * @code
 * template <>
 * struct SerializableType<ExternalData> {
 *     static void serialize(const ExternalData& obj, Serializer& s) {
 *         s.add_value(obj.value);
 *     }
 * };
 * @endcode
 */
template <typename T>
concept ExternalSerializable = requires(const T& t, Serializer& s) {
    { Serializable<std::remove_cvref_t<T>>::serialize(t, s) } -> std::same_as<void>;
};

/**
 * @brief Concept for types with a SerializableType<T> specialization providing deserialize().
 *
 * This enables non-intrusive deserialization for types you don't control.
 *
 * Example:
 * @code
 * template <>
 * struct SerializableType<ExternalData> {
 *     static ExternalData deserialize(Deserializer& d) {
 *         ExternalData obj;
 *         d.get_value(obj.value);
 *         return obj;
 *     }
 * };
 * @endcode
 */
template <typename T>
concept ExternalDeserializable = requires(Deserializer& d) {
    { Serializable<std::remove_cvref_t<T>>::deserialize(d) } -> std::same_as<T>;
};

/**
 * @brief Base class for sequential binary serialization (stream-based).
 *
 * Performance-focused alternative to ArchiveObject. Writes values directly to a stream
 * in order without intermediate property trees.
 *
 * **Use for**: Network packets, cache files, performance-critical paths, fixed schemas
 * **Avoid for**: Config files, save games, human-readable formats (use ArchiveObject instead)
 *
 * **Supported types**: Scalars, strings, GLM vectors, std::vector, std::map, enums, custom types
 *
 * @code
 * BinarySerializer serializer(output_stream);
 * serializer.add_value(42);
 * serializer.add_value("hello");
 * serializer.add_value(glm::vec3(1, 2, 3));
 * @endcode
 *
 * @see Deserializer for reading data back
 */
class Serializer
{
public:
    virtual ~Serializer() = default;

    /**
     * @brief Serializes a scalar value (integral or floating-point).
     *
     * Writes fundamental numeric types directly to the stream with type metadata.
     * Supports all integral types (int8_t through int64_t, unsigned variants) and
     * floating-point types (float, double).
     *
     * @tparam T Scalar type (integral or floating-point, excluding bool which has its own overload)
     * @param t The value to serialize
     */
    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::get_property_type<std::remove_const_t<T>>(),
                reflection::PropertyContainerType::scalar,
                1
            }
        );
    }

    /**
     * @brief Serializes a 128-bit unsigned integer.
     *
     * Special handling for uint128_t which requires explicit PropertyType::integer128.
     *
     * @param t The 128-bit integer to serialize
     */
    template <typename T> requires std::is_same_v<T, uint128_t>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::PropertyType::integer128,
                reflection::PropertyContainerType::scalar,
                1
            }
        );
    }


    /**
     * @brief Serializes a std::vector of complex elements. (not fundamental)
     *
     * Writes the vector size followed by each element. Each element's serialize() method
     * is called to write its data sequentially.
     *
     * @tparam T Vector type with complex elements
     * @param t The vector to serialize
     */
    template <reflection::Vector T> requires (!reflection::IsFundamental<typename T::value_type>)
    void add_value(const T& t)
    {
        const size_t size = t.size();
        add_value(size);

        for (const auto& value : t)
        {
            using ValueType = std::remove_cvref_t<typename T::value_type>;
            if constexpr (std::is_same_v<ValueType, StringId>)
                add_value(value);
            else
                add_value<ValueType>(value);
        }
    }

    /**
     * @brief Serializes a std::vector of basic fundamental (ints, floats, etc...)
     *
     * Writes the vector's data contiguously without size prefix. For vectors of
     * Serializable types, see the overload that writes size followed by elements.
     *
     * @tparam T Vector type with non-Serializable elements
     * @param t The vector to serialize
     */
    template <reflection::Vector T> requires reflection::IsFundamental<typename T::value_type>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t.data())), t.size() * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::array,
                t.size()
            }
        );
    }

    /**
     * @brief Serializes a string value.
     *
     * Writes the string with null terminator for safe deserialization. Handles std::string
     * and other types satisfying the reflection::String concept.
     *
     * @tparam T String type (std::string, etc.)
     * @param t The string to serialize
     */
    template <reflection::String T>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t.data())), (t.size() * sizeof(typename T::value_type)) + 1},
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                t.size() + 1
            }
        );
    }

    /**
     * @brief Serializes a std::string_view.
     *
     * Writes the string_view with null terminator.
     *
     * @param string_view The string view to serialize
     */
    void add_value(const std::string_view string_view)
    {
        add_property(
            reflection::Property{
                Buffer{
                    const_cast<void*>(static_cast<const void*>(string_view.data())),
                    (string_view.size() * sizeof(typename std::string_view::value_type)) + 1
                },
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                string_view.size() + 1
            }
        );
    }

    /**
     * @brief Serializes a GLM vec1 (single-component vector).
     *
     * @tparam T GLM vec1 type (glm::vec1, glm::ivec1, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec1 T>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                1
            }
        );
    }

    /**
     * @brief Serializes a GLM vec2 (two-component vector).
     *
     * @tparam T GLM vec2 type (glm::vec2, glm::ivec2, glm::dvec2, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec2 T>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 2 * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                2
            }
        );
    }

    /**
     * @brief Serializes a GLM vec3 (three-component vector).
     *
     * @tparam T GLM vec3 type (glm::vec3, glm::ivec3, glm::dvec3, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec3 T>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 3 * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                3
            }
        );
    }

    /**
     * @brief Serializes a GLM vec4 (four-component vector).
     *
     * @tparam T GLM vec4 type (glm::vec4, glm::ivec4, glm::dvec4, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec4 T>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 4 * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                4
            }
        );
    }

    /**
     * TODO ADD DOCSTRING
     */
    template <reflection::IsMatrix T>
    void add_value(const T& t)
    {
        constexpr auto element_number = T::length() * T::col_type::length();
        constexpr auto value_size = sizeof(typename T::value_type);

        add_property(
            reflection::Property{
                Buffer{&t[0][0], element_number * value_size},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::matrix,
                element_number
            }
        );
    }

    /**
     * @brief Serializes a map (std::map, std::unordered_map, etc.).
     *
     * Writes the map size followed by key-value pairs in sequence. Both keys and values
     * are serialized using their respective add_value overloads.
     *
     * @tparam T Map type
     * @param t The map to serialize
     */
    template <reflection::Map T>
    void add_value(const T& t)
    {
        const size_t size = t.size();
        add_value(size);

        for (const auto& [key, value] : t)
        {
            using KeyType = std::remove_const_t<typename T::key_type>;
            using ValueType = std::remove_const_t<typename T::mapped_type>;

            add_value<KeyType>(key);
            add_value<ValueType>(value);
        }
    }

    /**
     * @brief Serializes an enum as its underlying integer type.
     *
     * Converts the enum to its underlying type (e.g., int, uint8_t) and serializes
     * that value. More compact than the string-based enum serialization used by ArchiveObject.
     *
     * @tparam T Enum type
     * @param t The enum value to serialize
     */
    template <typename T> requires std::is_enum_v<T>
    void add_value(const T& t)
    {
        add_value<std::underlying_type_t<T>>(static_cast<std::underlying_type_t<T>>(t));
    }

    /**
     * @brief Serializes a custom Serializable type.
     *
     * Calls the type's serialize() method, which should write its members sequentially
     * to this Serializer.
     *
     * @tparam T Type satisfying the Serializable concept
     * @param t The object to serialize
     */
    template <SerializableConcept T>
    void add_value(const T& t)
    {
        t.serialize(*this);
    }

    /**
     * @brief Serializes a type with a SerializableType<T> specialization.
     *
     * Calls SerializableType<T>::serialize() to write the object's data sequentially.
     * This enables non-intrusive serialization for types you don't control.
     *
     * @tparam T Type satisfying the ExternalSerializable concept
     * @param t The object to serialize
     */
    template <ExternalSerializable T>
    void add_value(const T& t)
    {
        Serializable<std::remove_cvref_t<T>>::serialize(t, *this);
    }

    /**
     * TODO ADD DOCSTRING
     */
    template <typename T>
    void add_value(const std::optional<T>& optional_t)
    {
        add_value(optional_t.has_value());
        if (optional_t.has_value())
            add_value(optional_t.value());
    }

    /**
     * @brief Serializes types with glaze reflection metadata.
     *
     * Fallback overload for types with glaze compile-time reflection. Automatically
     * serializes all reflected fields in order. Use this for types that don't implement
     * serialize() but have glaze metadata.
     *
     * @tparam T Type with glaze reflection metadata (glz::reflect)
     * @param t The object to serialize
     */
    template <typename T> requires glz::reflectable<T> && (!SerializableConcept<T> && !ExternalSerializable<T>)
    void add_value(const T& t)
    {
        glz::for_each_field(
            t,
            [&](auto&& field)
            {
                add_value(field);
            }
        );
    }

    /**
     * @brief Serializes a C-string.
     *
     * Writes the string with null terminator. Calculates length using strlen().
     *
     * @param t Null-terminated C-string
     */
    void add_value(const char* t)
    {
        const size_t length = strlen(t);
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t)), (length * sizeof(char)) + 1},
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                length + 1
            }
        );
    }

    /**
     * @brief Serializes a raw binary data block.
     *
     * Writes arbitrary binary data of the specified length without interpretation.
     *
     * @param t Pointer to binary data
     * @param length Size of the binary data in bytes
     */
    void add_value(void* t, const size_t length)
    {
        add_property(reflection::Property{Buffer{t, length}, reflection::PropertyType::binary, reflection::PropertyContainerType::string, length});
    }

    /**
     * @brief Serializes a string id
     *
     * A specialization of `add_value` to serialize string id, serializes only the id part, without the string to save on space,
     *
     * @param id The string id to serialize
     */
    void add_value(const StringId& id)
    {
        add_value(id.id);
    }

protected:
    virtual void add_property(reflection::Property property) = 0;
};

/**
 * @brief Base class for sequential binary deserialization (stream-based).
 *
 * Counterpart to Serializer. Reads values from a binary stream in the exact order they
 * were written. The read order must match the write order precisely.
 *
 * @code
 * BinaryDeserializer deserializer(input_stream);
 * int value;
 * std::string text;
 * deserializer.get_value(value);
 * deserializer.get_value(text);
 * @endcode
 *
 * **Critical**: Read order must match write order exactly. No property names to match against.
 *
 * @see Serializer for writing data
 */
class Deserializer
{
public:
    virtual ~Deserializer() = default;

    /**
     * @brief Deserializes a scalar value (integral or floating-point).
     *
     * Reads fundamental numeric types from the stream. Validates type and size metadata
     * to catch deserialization errors in debug builds.
     *
     * @tparam T Scalar type (integral or floating-point, excluding bool)
     * @param t Output parameter to store the deserialized value
     */
    template <typename T> requires std::integral<T> || std::floating_point<T>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_ASSERT(property.type == reflection::get_property_type<T>(), "Property type mismatch");
        PORTAL_ASSERT(property.value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), property.value.size);

        t = *property.value.as<T*>();
    }

    /**
     * @brief Deserializes a 128-bit unsigned integer.
     *
     * Special handling for uint128_t with PropertyType::integer128 validation.
     *
     * @param t Output parameter to store the 128-bit integer
     */
    template <typename T> requires std::is_same_v<T, uint128_t>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_ASSERT(property.type == reflection::PropertyType::integer128, "Property type mismatch");
        PORTAL_ASSERT(property.value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), property.value.size);

        t = *property.value.as<T*>();
    }

    /**
     * @brief Deserializes a std::vector of complex elements.
     *
     * Reads the vector size followed by each element. Each element is deserialized in order.
     *
     * @tparam T Vector type with complex elements
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::Vector T> requires (!reflection::IsFundamental<typename T::value_type>)
    void get_value(T& t)
    {
        size_t size;
        get_value<size_t>(size);

        if constexpr (requires { t.reserve(size); })
        {
            t.reserve(size);
        }

        for (size_t i = 0; i < size; ++i)
        {
            typename T::value_type value;
            if constexpr (std::is_same_v<typename T::value_type, StringId>)
                get_value(value);
            else
                get_value<typename T::value_type>(value);
            t.push_back(std::move(value));
        }
    }

    /**
     * @brief Deserializes a std::vector of fundamental elements.
     *
     * Reads contiguous array data from the stream and constructs a vector from it.
     * For vectors of fundamental types, see the overload that reads size first.
     *
     * @tparam T Vector type with fundamental elements
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::Vector T> requires reflection::IsFundamental<typename T::value_type>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::array, "Property container type mismatch");
        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");

        auto array_length = property.elements_number;
        auto* data = property.value.as<typename T::value_type*>();
        t = T(data, data + array_length);
    }

    /**
     * @brief Deserializes a string value.
     *
     * Reads a string from the stream, handling both null-terminated and length-prefixed formats.
     *
     * @tparam T String type (std::string, etc.)
     * @param t Output parameter to store the deserialized string
     */
    template <reflection::String T>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::PropertyType::character, "Property type mismatch");

        size_t string_length = 0;
        if (property.container_type == reflection::PropertyContainerType::null_term_string)
            string_length = property.elements_number - 1;
        else if (property.container_type == reflection::PropertyContainerType::string)
            string_length = property.elements_number;

        const auto* data = property.value.as<const typename T::value_type*>();
        t = T(data, string_length);
    }

    /**
     * @brief Deserializes GLM vectors
     *
     * @tparam T GLM vecB type (glm::vecN, glm::ivecB, etc.)
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::IsVec T>
    void get_value(T& t)
    {
        constexpr auto element_number = T::length();

        auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == element_number, "Property elements number mismatch");

        t = T(*property.value.as<T*>());
    }

    /**
     * TODO ADD DOCSTRING
     */
    template <reflection::IsMatrix T>
    bool get_value(T& out)
    {
        [[maybe_unused]] constexpr auto element_number = T::length() * T::col_type::length();

        const auto& property = get_property();
        if (property.type == reflection::PropertyType::invalid)
            return false;

        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::matrix, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == element_number, "Property elements number mismatch");

        out = T(*property.value.as<T*>());
        return true;
    }

    /**
     * @brief Deserializes a map (std::map, std::unordered_map, etc.).
     *
     * Reads the map size followed by key-value pairs. Clears the output map and
     * reserves space if the map type supports it.
     *
     * @tparam T Map type
     * @param t Output parameter to store the deserialized map
     */
    template <reflection::Map T>
    void get_value(T& t)
    {
        size_t size;
        get_value<size_t>(size);

        t.clear();
        if constexpr (requires { t.reserve(size); })
        {
            t.reserve(size);
        }

        for (size_t i = 0; i < size; ++i)
        {
            typename T::key_type key;
            typename T::mapped_type value;
            get_value(key);
            get_value(value);
            t.insert_or_assign(std::move(key), std::move(value));
        }
    }


    /**
     * @brief Deserializes an enum from its underlying integer type.
     *
     * Reads the underlying type (e.g., int, uint8_t) and casts it back to the enum type.
     * Must match the serialization order exactly.
     *
     * @tparam T Enum type
     * @param t Output parameter to store the deserialized enum value
     */
    template <typename T> requires std::is_enum_v<T>
    void get_value(T& t)
    {
        std::underlying_type_t<T> underlying;
        get_value(underlying);
        t = static_cast<T>(underlying);
    }

    /**
     * @brief Deserializes a custom Deserializable type.
     *
     * Calls the type's static deserialize() method, which should read its members
     * sequentially from this Serializer and construct an instance.
     *
     * @tparam T Type satisfying the Deserializable concept
     * @param t Output parameter to store the deserialized object
     */
    template <DeserializableConcept T>
    void get_value(T& t)
    {
        t = T::deserialize(*this);
    }

    /**
     * @brief Deserializes a type with a SerializableType<T> specialization.
     *
     * Calls SerializableType<T>::deserialize() to read the object's data sequentially.
     * This enables non-intrusive deserialization for types you don't control.
     *
     * @tparam T Type satisfying the ExternalDeserializable concept
     * @param t Output parameter to store the deserialized object
     */
    template <ExternalDeserializable T>
    void get_value(T& t)
    {
        t = Serializable<std::remove_cvref_t<T>>::deserialize(*this);
    }

    /**
     * TODO ADD DOCSTRING
     */
    template <typename T>
    void get_value(std::optional<T>& optional_t)
    {
        bool has_value;
        get_value<bool>(has_value);

        if (has_value)
        {
            T underlying;
            if constexpr (std::is_same_v<T, StringId>)
                get_value(underlying);
            else
                get_value<T>(underlying);
            optional_t = std::optional<T>{std::move(underlying)};
        }
        else
            optional_t = std::nullopt;
    }

    /**
     * @brief Deserializes types with glaze reflection metadata (return-type version).
     *
     * Fallback overload for types with glaze compile-time reflection. Automatically
     * deserializes all reflected fields and returns a new instance. Use this for types
     * that don't implement deserialize() but have glaze metadata.
     *
     * @tparam T Type with glaze reflection metadata (glz::reflect)
     * @return The deserialized object
     */
    template <typename T> requires glz::reflectable<T> && (!DeserializableConcept<T> && !ExternalDeserializable<T>)
    void get_value(T& output)
    {
        constexpr auto N = glz::reflect<T>::size;

        if constexpr (N > 0)
        {
            glz::for_each<N>(
                [&]<size_t I>()
                {
                    auto& field = glz::get_member(output, glz::get<I>(glz::to_tie(output)));
                    get_value<std::remove_cvref_t<decltype(field)>>(field);
                }
            );
        }
    }

    /**
     * @brief Deserializes a C-string into a pre-allocated buffer.
     *
     * Reads character data from the stream and copies it into the provided buffer.
     * The buffer must be large enough to hold the expected string length.
     *
     * @param t Pointer to pre-allocated character buffer
     * @param length Expected length of the string (including null terminator)
     */
    void get_value(char*& t, const size_t length)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::PropertyType::character, "Property type mismatch");

        PORTAL_ASSERT(property.elements_number == length, "Value size mismatch, expected: {} got {}", length, property.elements_number);
        memcpy(t, property.value.data, (std::min)(length, property.elements_number));
    }

    /**
     * @brief Deserializes a StringId.
     *
     * A specialization of `get_value` to return a string id, deserializes only the ID part and looks up the string in the registry
     *
     * @param out Output variable, the string id to deserialize to
     */
    void get_value(StringId& out)
    {
        StringId::HashType id = 0;
        get_value(id);
        out = StringId{id};
    }

protected:
    virtual reflection::Property get_property() = 0;
};
} // namespace portal

template <portal::SerializableConcept T>
portal::Serializer& operator<<(portal::Serializer& s, const T& t)
{
    t.serialize(s);
    return s;
}

template <portal::reflection::PropertyConcept T>
portal::Serializer& operator<<(portal::Serializer& s, const T& t)
{
    T copy = t;
    s.add_value<T>(copy);
    return s;
}

template <portal::reflection::PropertyConcept T>
portal::Serializer& operator<<(portal::Serializer& s, T& t)
{
    s.add_value<T>(t);
    return s;
}

template <typename T> requires std::is_enum_v<T>
portal::Serializer& operator<<(portal::Serializer& s, const T& value)
{
    return s << static_cast<std::underlying_type_t<T>>(value);
}

inline portal::Serializer& operator<<(portal::Serializer& s, const char* str)
{
    s.add_value(str);
    return s;
}

template <typename T>
portal::Serializer* operator<<(portal::Serializer* s, const T& t)
{
    return &(*s << t);
}


template <portal::DeserializableConcept T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    t = T::deserialize(d);
    return d;
}

template <portal::reflection::PropertyConcept T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    d.get_value<T>(t);
    return d;
}

template <typename T> requires std::is_enum_v<T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    std::underlying_type_t<T> underlying;
    d >> underlying;
    t = static_cast<T>(underlying);
    return d;
}
