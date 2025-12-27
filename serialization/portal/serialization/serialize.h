//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/log.h>
#include <portal/core/reflection/property.h>

#include "portal/core/reflection/property_concepts.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
class Serializer;
class Deserializer;

/**
 * @brief Concept for types that can be serialized using the stream pattern.
 *
 * A type satisfies Serializable if it provides a serialize() method that writes
 * its data sequentially to a Serializer stream.
 *
 * Requirements:
 * - Must have a member function `void serialize(Serializer& s) const`
 *
 * Example:
 * @code
 * struct NetworkPacket {
 *     uint32_t sequence_id;
 *     std::string payload;
 *
 *     void serialize(Serializer& s) const {
 *         s.add_value(sequence_id);
 *         s.add_value(payload);
 *     }
 * };
 * @endcode
 *
 * @tparam T The type to check for serializability.
 * @see Archiveable for the visitor pattern alternative
 */
template <typename T>
concept Serializable = requires(const T t, Serializer& s) {
    { t.serialize(s) } -> std::same_as<void>;
};

/**
 * @brief Concept for types that can be deserialized using the stream pattern.
 *
 * A type satisfies Deserializable if it provides a static deserialize() method
 * that reads data sequentially from a Deserializer stream. The read order must
 * exactly match the write order from serialize().
 *
 * Requirements:
 * - Must have a static member function `T deserialize(Deserializer& d)`
 *
 * Example:
 * @code
 * struct NetworkPacket {
 *     static NetworkPacket deserialize(Deserializer& d) {
 *         NetworkPacket p;
 *         d.get_value(p.sequence_id);
 *         d.get_value(p.payload);
 *         return p;
 *     }
 * };
 * @endcode
 *
 * @tparam T The type to check for deserializability.
 * @see Dearchiveable for the visitor pattern alternative
 */
template <typename T>
concept Deserializable = requires(T t, Deserializer& d) {
    { T::deserialize(d) } -> std::same_as<T>;
};

/**
 * @brief Abstract base class for sequential binary serialization (stream-based pattern).
 *
 * Serializer is Portal's performance-focused alternative to ArchiveObject. Unlike the visitor
 * pattern (ArchiveObject) which builds an intermediate property tree, Serializer writes values
 * directly to a stream in a specific order, minimizing allocations and enabling efficient
 * network/cache serialization.
 *
 * ## When to Use Serializer vs ArchiveObject
 *
 * - **Use Serializer**: Network packets, cache files, performance-critical paths, fixed schemas
 * - **Use ArchiveObject**: Config files, save games, human-readable formats, flexible schemas
 *
 * ## Supported Types
 *
 * Serializer handles:
 * - Scalars: All integral types, float, double, bool
 * - Strings: std::string, C-strings (with length prefix)
 * - GLM vectors: vec1/2/3/4 variants
 * - Containers: std::vector, std::map (serializes size followed by elements)
 * - Enums: Stored as their underlying integer type
 * - Custom types: Any type implementing the Serializable concept
 * - StringId: Special handling to preserve both hash and string
 *
 * ## Usage Pattern
 *
 * @code
 * BinarySerializer serializer(output_stream);
 * serializer.add_value(42);
 * serializer.add_value("hello");
 * serializer.add_value(glm::vec3(1, 2, 3));
 *
 * // For custom Serializable types, both approaches work:
 * serializer.add_value(my_custom_object);  // Automatically calls serialize()
 * // Or explicitly:
 * my_custom_object.serialize(serializer);
 * @endcode
 *
 * ## Critical Note
 *
 * Write and read order must match **exactly**. There are no property names to match against.
 *
 * @see Deserializer for reading data back
 * @see BinarySerializer for the default implementation
 * @see ArchiveObject for named-property serialization
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
    template <typename T>
        requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::get_property_type<std::remove_const_t<T>>(),
                reflection::PropertyContainerType::scalar,
                1});
    }

    /**
     * @brief Serializes a 128-bit unsigned integer.
     *
     * Special handling for uint128_t which requires explicit PropertyType::integer128.
     *
     * @param t The 128-bit integer to serialize
     */
    template <typename T>
        requires std::is_same_v<T, uint128_t>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::PropertyType::integer128,
                reflection::PropertyContainerType::scalar,
                1});
    }

    /**
     * @brief Serializes a std::vector of non-Serializable elements.
     *
     * Writes the vector's data contiguously without size prefix. For vectors of
     * Serializable types, see the overload that writes size followed by elements.
     *
     * @tparam T Vector type with non-Serializable elements
     * @param t The vector to serialize
     */
    template <reflection::Vector T>
        requires(!Serializable<typename T::value_type>)
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t.data())), t.size() * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::array,
                t.size()});
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
                t.size() + 1});
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
                    (string_view.size() * sizeof(typename std::string_view::value_type)) + 1},
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                string_view.size() + 1});
    }

    /**
     * @brief Serializes a GLM vec1 (single-component vector).
     *
     * @tparam T GLM vec1 type (glm::vec1, glm::ivec1, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec1 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                1});
    }

    /**
     * @brief Serializes a GLM vec2 (two-component vector).
     *
     * @tparam T GLM vec2 type (glm::vec2, glm::ivec2, glm::dvec2, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec2 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 2 * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                2});
    }

    /**
     * @brief Serializes a GLM vec3 (three-component vector).
     *
     * @tparam T GLM vec3 type (glm::vec3, glm::ivec3, glm::dvec3, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec3 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 3 * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                3});
    }

    /**
     * @brief Serializes a GLM vec4 (four-component vector).
     *
     * @tparam T GLM vec4 type (glm::vec4, glm::ivec4, glm::dvec4, etc.)
     * @param t The vector to serialize
     */
    template <reflection::GlmVec4 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 4 * sizeof(typename T::value_type)},
                reflection::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                4});
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
     * @brief Serializes a std::vector of Serializable elements.
     *
     * Writes the vector size followed by each element. Each element's serialize() method
     * is called to write its data sequentially.
     *
     * @tparam T Vector type with Serializable elements
     * @param t The vector to serialize
     */
    template <reflection::Vector T>
        requires Serializable<typename T::value_type>
    void add_value(const T& t)
    {
        const size_t size = t.size();
        add_value(size);

        for (const auto& value : t)
        {
            using ValueType = std::remove_const_t<typename T::value_type>;
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
    template <typename T>
        requires std::is_enum_v<T>
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
    template <Serializable T>
    void add_value(const T& t)
    {
        t.serialize(*this);
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
                length + 1});
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
     * @brief Serializes a StringId preserving both hash and string.
     *
     * Writes both the string content and the hash ID to enable debug information
     * to survive serialization round-trips.
     *
     * @param string_id The StringId to serialize
     * @note TODO: Consider using a string map to deduplicate strings and store only IDs
     */
    void add_value(const StringId& string_id)
    {
        add_value(string_id.string);
        add_value(string_id.id);
    }

    /**
     * @brief Pure virtual method for derived classes to implement stream writing.
     *
     * Concrete implementations (like BinarySerializer) encode the Property metadata
     * and value into their specific binary format.
     *
     * @param property Property containing value buffer, type metadata, and element count
     */
    virtual void add_property(reflection::Property property) = 0;
};

/**
 * @brief Abstract base class for sequential binary deserialization (stream-based pattern).
 *
 * Deserializer is the reading counterpart to Serializer. It reads values sequentially
 * from a binary stream, with **absolutely critical requirement**: the read order must
 * match the write order exactly, as there are no property names to identify fields.
 *
 * ## Read Order Requirement
 *
 * @code
 * // Serialization
 * serializer.add_value(player_health);
 * serializer.add_value(player_position);
 * serializer.add_value(player_name);
 *
 * // Deserialization - MUST match order
 * deserializer.get_value(player_health);    // OK
 * deserializer.get_value(player_position);  // OK
 * deserializer.get_value(player_name);      // OK
 *
 * // Deserialization - WRONG order = corruption
 * deserializer.get_value(player_name);      // WRONG!
 * deserializer.get_value(player_health);    // WRONG!
 * @endcode
 *
 * ## Error Handling
 *
 * - **Type mismatches**: Caught in debug builds via assertions. Release builds may produce
 *   undefined behavior if types don't match. Ensure your serialize/deserialize code stays
 *   perfectly synchronized.
 * - **Stream errors**: Not automatically detected. Check the input stream state after
 *   deserialization completes:
 *   @code
 *   deserializer.get_value(data);
 *   if (input_stream.fail() || input_stream.eof()) {
 *       // Handle malformed data or premature end of stream
 *   }
 *   @endcode
 *   Premature stream end or read errors will cause undefined behavior or crashes without
 *   diagnostic information.
 *
 * ## Usage Pattern
 *
 * @code
 * BinaryDeserializer deserializer(input_stream);
 * int value;
 * std::string text;
 * glm::vec3 position;
 * deserializer.get_value(value);
 * deserializer.get_value(text);
 * deserializer.get_value(position);
 * @endcode
 *
 * @see Serializer for writing data
 * @see BinaryDeserializer for the default implementation
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
    template <typename T>
        requires std::integral<T> || std::floating_point<T>
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
    template <typename T>
        requires std::is_same_v<T, uint128_t>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_ASSERT(property.type == reflection::PropertyType::integer128, "Property type mismatch");
        PORTAL_ASSERT(property.value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), property.value.size);

        t = *property.value.as<T*>();
    }

    /**
     * @brief Deserializes a std::vector of non-Serializable elements.
     *
     * Reads contiguous array data from the stream and constructs a vector from it.
     * For vectors of Serializable types, see the overload that reads size first.
     *
     * @tparam T Vector type with non-Serializable elements
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::Vector T>
        requires(!Serializable<typename T::value_type>)
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
     * @brief Deserializes a GLM vec1 (single-component vector).
     *
     * @tparam T GLM vec1 type (glm::vec1, glm::ivec1, etc.)
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::GlmVec1 T>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 1, "Property elements number mismatch");

        t = T(*property.value.as<typename T::value_type*>());
    }

    /**
     * @brief Deserializes a GLM vec2 (two-component vector).
     *
     * @tparam T GLM vec2 type (glm::vec2, glm::ivec2, glm::dvec2, etc.)
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::GlmVec2 T>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 2, "Property elements number mismatch");

        const auto* data = property.value.as<const typename T::value_type*>();
        t = T(data[0], data[1]);
    }

    /**
     * @brief Deserializes a GLM vec3 (three-component vector).
     *
     * @tparam T GLM vec3 type (glm::vec3, glm::ivec3, glm::dvec3, etc.)
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::GlmVec3 T>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 3, "Property elements number mismatch");

        const auto* data = property.value.as<const typename T::value_type*>();
        t = T(data[0], data[1], data[2]);
    }

    /**
     * @brief Deserializes a GLM vec4 (four-component vector).
     *
     * @tparam T GLM vec4 type (glm::vec4, glm::ivec4, glm::dvec4, etc.)
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::GlmVec4 T>
    void get_value(T& t)
    {
        auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 4, "Property elements number mismatch");

        const auto* data = property.value.as<const typename T::value_type*>();
        t = T(data[0], data[1], data[2], data[3]);
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
     * @brief Deserializes a std::vector of Serializable elements.
     *
     * Reads the vector size followed by each element. Each element's deserialize()
     * static method is called to reconstruct it from the stream.
     *
     * @tparam T Vector type with Serializable elements
     * @param t Output parameter to store the deserialized vector
     */
    template <reflection::Vector T>
        requires Serializable<typename T::value_type>
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
            get_value<typename T::value_type>(value);
            t.push_back(std::move(value));
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
    template <typename T>
        requires std::is_enum_v<T>
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
     * sequentially from this Deserializer and construct an instance.
     *
     * @tparam T Type satisfying the Deserializable concept
     * @param t Output parameter to store the deserialized object
     */
    template <Deserializable T>
    void get_value(T& t)
    {
        t = T::deserialize(*this);
    }

    /**
     * @brief Template helper to deserialize and return a value by type.
     *
     * Convenience method that creates a default-constructed instance and deserializes
     * into it, then returns the value.
     *
     * @tparam T Type to deserialize
     * @return The deserialized value
     */
    template <typename T>
    T get_value()
    {
        T t;
        get_value<T>(t);
        return t;
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

protected:
    virtual reflection::Property get_property() = 0;
};

template <>
inline StringId Deserializer::get_value<StringId>()
{
    std::string string;
    uint64_t id;

    get_value(string);
    get_value(id);

    return StringId{id, string};
}
} // namespace portal

template <portal::Serializable T>
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

template <typename T>
    requires std::is_enum_v<T>
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


template <portal::Deserializable T>
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

template <typename T>
    requires std::is_enum_v<T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    std::underlying_type_t<T> underlying;
    d >> underlying;
    t = static_cast<T>(underlying);
    return d;
}
