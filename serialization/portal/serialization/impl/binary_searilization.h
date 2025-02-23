//
// Created by Jonatan Nevo on 22/02/2025.
//

#pragma once
#include "portal/serialization/serialization.h"

namespace portal
{

struct BinarySerializationParams
{
    bool encode_params = true;
    bool large_element_size = false;
};

class BinarySerializer final : public serialization::OrderedSerializer
{
public:
    BinarySerializer(std::ostream& output, std::optional<BinarySerializationParams> param = std::nullopt);
    void serialize() override;

private:
    std::optional<BinarySerializationParams> params;
    std::ostream& output;
};

class BinaryDeserializer final : public serialization::OrderedDeserializer
{
public:
    BinaryDeserializer(std::istream& input, std::optional<BinarySerializationParams> params = std::nullopt);
    BinaryDeserializer(void* buffer, size_t size, std::optional<BinarySerializationParams> params = std::nullopt);

    ~BinaryDeserializer() override;
    void deserialize() override;

private:
    std::optional<BinarySerializationParams> params;
    bool needs_free = false;
    uint8_t* buffer = nullptr;
    size_t size = 0;
};

} // namespace portal
