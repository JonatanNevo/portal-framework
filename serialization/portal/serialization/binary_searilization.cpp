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

BinarySerializer::BinarySerializer(std::ostream& output) : output(output) {}

void BinarySerializer::serialize()
{
    for (const auto& [_, property] : properties)
    {
        output.write(reinterpret_cast<const char*>(&property.container_type), 1);
        output.write(reinterpret_cast<const char*>(&property.type), 1);
        if (property.container_type != serialization::PropertyContainerType::scalar)
        {
            if (property.container_type == serialization::PropertyContainerType::vector)
                output.write(reinterpret_cast<const char*>(&property.elements_number), sizeof(uint8_t));
            else
                output.write(reinterpret_cast<const char*>(&property.elements_number), sizeof(uint16_t));
        }
        output.write(static_cast<const char*>(property.value.data), property.value.size);
    }
}

BinaryDeserializer::BinaryDeserializer(std::istream& input)
{
    input.seekg(0, std::ios::end);
    size = input.tellg();
    input.seekg(0, std::ios::beg);
    const auto new_buffer = new char[size];
    input.read(new_buffer, size);
    buffer = reinterpret_cast<uint8_t*>(new_buffer);
    needs_free = true;
}

BinaryDeserializer::BinaryDeserializer(void* buffer, const size_t size) :
    buffer(static_cast<uint8_t*>(buffer)), size(size)
{
}

BinaryDeserializer::~BinaryDeserializer()
{
    if (buffer != nullptr && needs_free)
        delete[] buffer;
}

void BinaryDeserializer::deserialize()
{
    for (int i = 0; i < size;)
    {
        const auto container_type = static_cast<serialization::PropertyContainerType>(buffer[i++]);
        const auto type = static_cast<serialization::PropertyType>(buffer[i++]);
        const auto element_size = get_size(type);

        uint16_t elements_number = 1;
        if (container_type != serialization::PropertyContainerType::scalar)
        {
            if (container_type == serialization::PropertyContainerType::vector)
            {
                elements_number = buffer[i];
                i += sizeof(uint8_t);
            }
            else
            {
                elements_number = static_cast<uint16_t>(buffer[i]);
                i += sizeof(uint16_t);
            }
        }

        const Buffer value{buffer + i, elements_number * element_size};
        i += static_cast<int>(elements_number * element_size);
        properties[std::to_string(counter++)] = {value, type, container_type, elements_number};
    }
    counter = 0;
}

} // namespace portal
