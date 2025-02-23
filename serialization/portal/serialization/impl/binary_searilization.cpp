//
// Created by Jonatan Nevo on 22/02/2025.
//

#include "binary_searilization.h"

#include <ranges>

namespace portal
{
size_t get_size(const serialization::PropertyType type)
{
    switch (type)
    {
    case serialization::PropertyType::binary:
    case serialization::PropertyType::character:
    case serialization::PropertyType::integer8:
        return 1;
    case serialization::PropertyType::integer16:
        return 2;
    case serialization::PropertyType::integer32:
    case serialization::PropertyType::floating32:
        return 4;
    case serialization::PropertyType::integer64:
    case serialization::PropertyType::floating64:
        return 8;
    case serialization::PropertyType::integer128:
        return 16;
    case serialization::PropertyType::invalid:
        return 0;
    }
    return 0;
}

uint8_t element_number_size(const BinarySerializationParams& params)
{
    if (params.large_element_size)
        return sizeof(size_t);
    return sizeof(uint16_t);
}

/**
 * Turns BinarySerializationParams to a binary header with the following format:
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
uint8_t to_params_header(const BinarySerializationParams& params)
{
    uint8_t header = 0;
    header |= static_cast<uint8_t>(params.large_element_size);
    header |= static_cast<uint8_t>(params.encode_params << 7);
    return header;
}

BinarySerializationParams from_params_header(const uint8_t header)
{
    return BinarySerializationParams{
        .encode_params = static_cast<bool>(header >> 7),
        .large_element_size = static_cast<bool>(header & 0b1),
    };
}


BinarySerializer::BinarySerializer(std::ostream& output, const std::optional<BinarySerializationParams> params) :
    params(params), output(output)
{
}

void BinarySerializer::serialize()
{
    const auto params_value = params.value_or(BinarySerializationParams{});
    if (params_value.encode_params)
    {
        const uint8_t header = to_params_header(params_value);
        output.write(reinterpret_cast<const char*>(&header), 1);
    }

    for (const auto& [_, property] : properties)
    {
        output.write(reinterpret_cast<const char*>(&property.container_type), 1);
        output.write(reinterpret_cast<const char*>(&property.type), 1);
        if (property.container_type != serialization::PropertyContainerType::scalar &&
            !is_vector_type(property.container_type))
        {
            output.write(reinterpret_cast<const char*>(&property.elements_number), element_number_size(params_value));
        }
        output.write(static_cast<const char*>(property.value.data), property.value.size);
    }
}

BinaryDeserializer::BinaryDeserializer(std::istream& input, const std::optional<BinarySerializationParams> params) :
    params(params)
{
    input.seekg(0, std::ios::end);
    size = input.tellg();
    input.seekg(0, std::ios::beg);
    const auto new_buffer = new char[size];
    input.read(new_buffer, size);
    buffer = reinterpret_cast<uint8_t*>(new_buffer);
    needs_free = true;
}

BinaryDeserializer::BinaryDeserializer(
    void* buffer, const size_t size, const std::optional<BinarySerializationParams> params) :
    params(params), buffer(static_cast<uint8_t*>(buffer)), size(size)
{
}

BinaryDeserializer::~BinaryDeserializer()
{
    if (buffer != nullptr && needs_free)
        delete[] buffer;
}

void BinaryDeserializer::deserialize()
{
    int i = 0;
    const auto header = buffer[i];

    BinarySerializationParams params_value;
    if (params == std::nullopt)
    {
        params_value = from_params_header(header);
        if (!params_value.encode_params)
            params_value = BinarySerializationParams{.encode_params = false};
    }
    else
        params_value = params.value();

    i += params_value.encode_params;
    while (i < size)
    {
        const auto container_type = static_cast<serialization::PropertyContainerType>(buffer[i++]);
        const auto type = static_cast<serialization::PropertyType>(buffer[i++]);
        const auto element_size = get_size(type);

        uint16_t elements_number = 1;
        if (container_type != serialization::PropertyContainerType::scalar)
        {
            if (is_vector_type(container_type))
            {
                elements_number = static_cast<uint8_t>(container_type) -
                    static_cast<uint8_t>(serialization::PropertyContainerType::__vector_type_start) + 1;
            }
            else
            {
                if (element_number_size(params_value) == sizeof(size_t))
                {
                    elements_number = static_cast<size_t>(buffer[i]);
                    i += sizeof(size_t);
                }
                else
                {
                    elements_number = static_cast<uint16_t>(buffer[i]);
                    i += sizeof(uint16_t);
                }
            }
        }

        const Buffer value{buffer + i, elements_number * element_size};
        i += static_cast<int>(elements_number * element_size);
        properties[std::to_string(counter++)] = {value, type, container_type, elements_number};
    }
    counter = 0;
}

} // namespace portal
