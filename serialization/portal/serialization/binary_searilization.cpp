//
// Created by Jonatan Nevo on 22/02/2025.
//

#include "binary_searilization.h"

#include <ranges>

namespace portal
{

BinarySerializer::BinarySerializer(std::ostream& output) : output(output) {}

void BinarySerializer::serialize()
{
    for (const auto& [_, property] : properties)
    {
        const bool container = property.container_type != serialization::PropertyContainerType::scalar;
        output.write(reinterpret_cast<const char*>(&container), 1);
        if (container)
            output.write(reinterpret_cast<const char*>(&property.container_type), 1);
        output.write(reinterpret_cast<const char*>(&property.type), 1);
        output.write(reinterpret_cast<const char*>(&property.value.size), sizeof(size_t));
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
BinaryDeserializer::BinaryDeserializer(void* buffer, const size_t size): buffer(static_cast<uint8_t*>(buffer)), size(size) {}

BinaryDeserializer::~BinaryDeserializer()
{
    if (buffer != nullptr && needs_free)
        delete[] buffer;
}

void BinaryDeserializer::deserialize()
{
    for (int i = 0; i < size;)
    {
        const bool container = static_cast<bool>(buffer[i++]);
        auto container_type = serialization::PropertyContainerType::scalar;
        if (container)
            container_type = static_cast<serialization::PropertyContainerType>(buffer[i++]);
        const auto type = static_cast<serialization::PropertyType>(buffer[i++]);
        const size_t value_size = static_cast<size_t>(buffer[i]);
        i += sizeof(size_t);

        const Buffer value{buffer + i, value_size};
        i += value_size;
        properties[std::to_string(counter++)] = {value, type, container_type};
    }
    counter = 0;
}

} // namespace portal
