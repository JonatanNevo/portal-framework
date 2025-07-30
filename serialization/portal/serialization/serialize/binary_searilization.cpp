//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "binary_searilization.h"

#include <ranges>

namespace portal
{
constexpr size_t get_size(const serialize::PropertyType type)
{
    switch (type)
    {
    case serialize::PropertyType::binary:
    case serialize::PropertyType::character:
    case serialize::PropertyType::integer8:
        return 1;
    case serialize::PropertyType::integer16:
        return 2;
    case serialize::PropertyType::integer32:
    case serialize::PropertyType::floating32:
        return 4;
    case serialize::PropertyType::integer64:
    case serialize::PropertyType::floating64:
        return 8;
    case serialize::PropertyType::integer128:
        return 16;
    case serialize::PropertyType::boolean:
        return 1;
    case serialize::PropertyType::object:
        [[fallthrough]];
    case serialize::PropertyType::null_term_string:
        [[fallthrough]];
    case serialize::PropertyType::string:
        [[fallthrough]];
    case serialize::PropertyType::invalid:
        return 0;
    }
    return 0;
}

constexpr uint8_t element_number_size(const BinarySerializationParams& params)
{
    if (params.large_element_size)
        return sizeof(size_t);
    return sizeof(uint16_t);
}


struct Header
{
    using HeaderSizeT = uint32_t;

    std::string_view magic = MAGIC;
    uint8_t version = VERSION;
    BinarySerializationParams params;

    [[nodiscard]] HeaderSizeT serialize() const
    {
        HeaderSizeT header = 0;
        header |= static_cast<uint8_t>(magic[0]);
        header |= static_cast<uint8_t>(magic[1]) << 8;
        header |= version << 16;
        header |= encode_params() << 24;
        return header;
    }

    static Header deserialize(const HeaderSizeT serialized)
    {
        char magic[2];
        magic[0] = static_cast<char>(serialized & 0xFF);
        magic[1] = static_cast<char>((serialized >> 8) & 0xFF);
        const auto version = static_cast<uint8_t>((serialized >> 16) & 0xFF);
        const auto params = decode_params(static_cast<uint8_t>((serialized >> 24) & 0xFF));
        const auto header = Header{
            .magic = std::string_view(magic, 2),
            .version = version,
            .params = params
        };
        validate_header(header);
        return header;
    }

    static void validate_header([[maybe_unused]] const Header& header)
    {
        PORTAL_ASSERT(header.magic == MAGIC, "Invalid serialized buffer magic number");
        PORTAL_ASSERT(header.version == VERSION, "Invalid serialized buffer magic version");
    }

private:
    /**
     * Turns BinarySerializationParams to a binary with the following format:
     * 0 - large element size flag
     * 1:6 - reserved
     * 7 - encode params header flag
     *
     * example:
     *  0b10000001
     *    s      l
     *
     *   s = header is encoded
     *   l = large element size
     *
     *   NOTE: the "encode_params" flag is always the 8th bit because no container type will reach this bit,
     *   allowing us to always know if it's an encoded header or not based on this bit
     */
    [[nodiscard]] uint8_t encode_params() const
    {
        uint8_t encoded = 0;
        encoded |= static_cast<uint8_t>(params.large_element_size);
        encoded |= static_cast<uint8_t>(params.encode_header << 7);
        return encoded;
    }

    static BinarySerializationParams decode_params(const uint8_t header)
    {
        return BinarySerializationParams{
            .encode_header = static_cast<bool>(header >> 7),
            .large_element_size = static_cast<bool>(header & 0b1),
        };
    }
};


BinarySerializer::BinarySerializer(std::ostream& output) :
    BinarySerializer(output, BinarySerializationParams{})
{
}

BinarySerializer::BinarySerializer(std::ostream& output, const BinarySerializationParams params) :
    params(params), output(output)
{
    if (params.encode_header)
    {
        const auto header = Header{.params = params}.serialize();
        output.write(reinterpret_cast<const char*>(&header), sizeof(Header::HeaderSizeT));
    }
}

void BinarySerializer::add_property(const serialize::Property property)
{
    output.write(reinterpret_cast<const char*>(&property.container_type), 1);
    output.write(reinterpret_cast<const char*>(&property.type), 1);
    if (property.container_type != serialize::PropertyContainerType::scalar &&
        !is_vector_type(property.container_type))
    {
        output.write(reinterpret_cast<const char*>(&property.elements_number), element_number_size(params));
    }
    output.write(static_cast<const char*>(property.value.data), static_cast<int64_t>(property.value.size));
}


BinaryDeserializer::BinaryDeserializer(std::istream& input, const bool read_header) :
    input(input)
{
    const auto size = input.seekg(0, std::ios::end).tellg();
    input.seekg(0, std::ios::beg);

    buffer.resize(size);

    if (read_header)
    {
        Header::HeaderSizeT encoded_header;
        input.read(reinterpret_cast<char*>(&encoded_header), sizeof(Header::HeaderSizeT));
        const auto header = Header::deserialize(encoded_header);
        params = header.params;
    }
    else
        params = BinarySerializationParams{};
}

BinaryDeserializer::BinaryDeserializer(std::istream& input, const BinarySerializationParams params):
    input(input), params(params)
{
    const auto size = input.seekg(0, std::ios::end).tellg();
    input.seekg(0, std::ios::beg);

    buffer.resize(size);
}

serialize::Property BinaryDeserializer::get_property()
{
    serialize::PropertyContainerType container_type;
    serialize::PropertyType type;
    input.read(reinterpret_cast<char*>(&container_type), 1);
    input.read(reinterpret_cast<char*>(&type), 1);
    const auto element_size = get_size(type);

    size_t elements_number = 1;
    if (container_type != serialize::PropertyContainerType::scalar)
    {
        if (is_vector_type(container_type))
            elements_number = static_cast<uint8_t>(container_type) -
                static_cast<uint8_t>(serialize::PropertyContainerType::__vector_type_start) + 1;
        else
            input.read(reinterpret_cast<char*>(&elements_number), element_number_size(params));
    }

    input.read(buffer.data() + cursor, static_cast<int64_t>(elements_number * element_size));
    const Buffer value{buffer.data() + cursor, elements_number * element_size};
    cursor += elements_number * element_size;

    return {
        .value = value,
        .type = type,
        .container_type = container_type,
        .elements_number = elements_number
    };
}

} // namespace portal

