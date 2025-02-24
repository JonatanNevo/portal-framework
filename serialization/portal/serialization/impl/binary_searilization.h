//
// Created by Jonatan Nevo on 22/02/2025.
//

#pragma once
#include "portal/serialization/serialize.h"

namespace portal
{

constexpr std::string_view MAGIC = "PS";
constexpr uint8_t VERSION = 1;

struct BinarySerializationParams
{
    bool encode_header = true;
    bool large_element_size = false;
};

class BinarySerializer final : public Serializer
{
public:
    explicit BinarySerializer(std::ostream& output);
    BinarySerializer(std::ostream& output, BinarySerializationParams params);

protected:
    void add_property(serialization::Property property) override;

private:
    BinarySerializationParams params;
    std::ostream& output;
};

class BinaryDeserializer final : public Deserializer
{
public:
    explicit BinaryDeserializer(std::istream& input, bool read_header = true);
    BinaryDeserializer(std::istream& input, BinarySerializationParams params);


protected:
    serialization::Property get_property() override;

private:
    std::istream& input;
    BinarySerializationParams params;

    std::vector<char> buffer;
    size_t cursor = 0;
};

} // namespace portal
