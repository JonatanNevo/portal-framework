//
// Created by Jonatan Nevo on 22/02/2025.
//

#pragma once
#include "portal/serialization/deserializer_base.h"
#include "portal/serialization/serializer_base.h"

namespace portal
{

class BinarySerializer final : public serialization::OrderedSerializer
{
public:
    explicit BinarySerializer(std::ostream& output);
    void serialize() override;

private:
    std::ostream& output;
};

class BinaryDeserializer final : public serialization::OrderedDeserializer
{
public:
    explicit BinaryDeserializer(std::istream& input);
    BinaryDeserializer(void* buffer, size_t size);

    ~BinaryDeserializer() override;
    void deserialize() override;

private:
    bool needs_free = false;
    uint8_t* buffer = nullptr;
    size_t size = 0;
};

} // namespace portal
