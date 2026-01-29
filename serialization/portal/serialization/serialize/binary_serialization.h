//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/serialization/serialize.h"

namespace portal
{
/** @brief Magic bytes identifying Portal Serialization format ("PS") */
constexpr std::array MAGIC = {'P', 'S'};

/** @brief Binary format version number (currently 1) */
constexpr uint8_t VERSION = 1;

/**
 * @brief Configuration parameters for binary serialization behavior.
 */
struct BinarySerializationParams
{
    /** @brief Whether to write/read the 4-byte header (magic + version). Default: true */
    bool encode_header = true;

    /** @brief Use 64-bit element counts instead of 32-bit. Default: false */
    bool large_element_size = false;

    /**
     * @brief Pack array elements without writing size metadata.
     *
     * When true, arrays are written without their element count. Requires external
     * knowledge of array sizes during deserialization. Default: false
     */
    bool pack_elements = false;
};

/**
 * @brief Concrete binary serialization implementation using a compact stream format.
 *
 * BinarySerializer writes data sequentially to an std::ostream in a custom binary format
 * optimized for compactness and speed.
 *
 * ## Binary Format Specification
 *
 * **Header** (4 bytes, if encode_header=true):
 * - Magic bytes: "PS" (0x50, 0x53)
 * - Version: 1 byte (currently 0x01)
 * - Reserved: 1 byte (0x00)
 *
 * **Per-value encoding**:
 * - Type metadata (2 bytes):
 *   - Byte 0: PropertyType enum value
 *   - Byte 1: PropertyContainerType enum value
 * - Size (4 or 8 bytes, for arrays/strings only):
 *   - Element count (uint32_t or uint64_t based on large_element_size)
 *   - Omitted for scalars and when pack_elements=true
 * - Data (variable):
 *   - Raw bytes in native endianness
 *
 * @see BinaryDeserializer for reading the binary format back
 * @see Serializer for the abstract interface
 */
class BinarySerializer final : public Serializer
{
public:
    explicit BinarySerializer(std::ostream& output);
    BinarySerializer(std::ostream& output, BinarySerializationParams params);

protected:
    void add_property(reflection::Property property) override;
    size_t reserve_slot(reflection::Property property) override;
    void write_at(size_t position, const void* data, size_t size) override;

private:
    BinarySerializationParams params;
    std::ostream& output;
};

/**
 * @brief Concrete binary deserialization implementation for reading Portal's binary format.
 *
 * BinaryDeserializer reads data sequentially from an std::istream in the binary format
 * written by BinarySerializer. The read order must **exactly** match the write order.
 *
 * ## Header Validation
 *
 * If encode_header=true (default), BinaryDeserializer validates:
 * - Magic bytes match "PS"
 * - Version is compatible (currently checks for version 1)
 *
 * Header mismatch triggers assertions in debug builds.
 *
 * ## Usage Example
 *
 * @code
 * std::ifstream file("data.bin", std::ios::binary);
 * BinaryDeserializer deserializer(file);
 *
 * int value;
 * std::string text;
 * glm::vec3 position;
 *
 * deserializer.get_value(value);     // Must match write order
 * deserializer.get_value(text);
 * deserializer.get_value(position);
 * @endcode
 *
 * ## Error Handling
 *
 * - **Type mismatches**: Caught by assertions in debug builds
 * - **Stream errors**: Check stream state after deserialization
 * - **Missing data**: May read garbage or crash if stream ends prematurely
 *
 * @see BinarySerializer for writing the binary format
 * @see Deserializer for the abstract interface
 */
class BinaryDeserializer final : public Deserializer
{
public:
    /**
     * @brief Constructs deserializer with default parameters.
     * @param input Input stream to read from (must be opened in binary mode)
     * @param read_header Whether to read and validate the 4-byte header
     */
    explicit BinaryDeserializer(std::istream& input, bool read_header = true);

    /**
     * @brief Constructs deserializer with custom parameters.
     * @param input Input stream to read from (must be opened in binary mode)
     * @param params Configuration matching the serialization parameters
     */
    BinaryDeserializer(std::istream& input, BinarySerializationParams params);

protected:
    reflection::Property get_property() override;

private:
    std::istream& input;
    BinarySerializationParams params;

    std::vector<char> buffer;
    size_t cursor = 0;
};
} // namespace portal
