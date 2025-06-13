//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "json_archive.h"

namespace portal
{
JsonArchiver::JsonArchiver(std::ostream& output):
    output(output) {}

void JsonArchiver::archive()
{
    output << archive_object;
}

void JsonArchiver::add_property(const std::string& name, const serialization::Property& property)
{
    switch (property.container_type)
    {
    case serialization::PropertyContainerType::scalar:
    {
        switch (property.type)
        {
        case serialization::PropertyType::integer8:
            archive_object[name] = *static_cast<uint8_t*>(property.value.data);
            break;
        case serialization::PropertyType::integer16:
            archive_object[name] = *static_cast<uint16_t*>(property.value.data);
            break;
        case serialization::PropertyType::integer32:
            archive_object[name] = *static_cast<uint32_t*>(property.value.data);
            break;
        case serialization::PropertyType::integer64:
            archive_object[name] = *static_cast<uint64_t*>(property.value.data);
            break;
        case serialization::PropertyType::floating32:
            archive_object[name] = *static_cast<float*>(property.value.data);
            break;
        case serialization::PropertyType::floating64:
            archive_object[name] = *static_cast<double*>(property.value.data);
            break;
        case serialization::PropertyType::character:
            archive_object[name] = *static_cast<char*>(property.value.data);
            break;
        case serialization::PropertyType::binary:
        case serialization::PropertyType::integer128:
        case serialization::PropertyType::invalid:
            LOG_ERROR_TAG("Json Archiver", "Invalid property type for scalar in property {}", name);
            break;
        }
        break;
    }
    case serialization::PropertyContainerType::array:
    {
        switch (property.type)
        {
        case serialization::PropertyType::integer8:
            archive_object[name] = std::vector<uint8_t>(
                static_cast<uint8_t*>(property.value.data),
                static_cast<uint8_t*>(property.value.data) + property.value.size / sizeof(uint8_t)
                );
            break;
        case serialization::PropertyType::integer16:
            archive_object = std::vector<uint16_t>(
                static_cast<uint16_t*>(property.value.data),
                static_cast<uint16_t*>(property.value.data) + property.value.size / sizeof(uint16_t)
                );
            break;
        case serialization::PropertyType::integer32:
            archive_object[name] = std::vector<uint32_t>(
                static_cast<uint32_t*>(property.value.data),
                static_cast<uint32_t*>(property.value.data) + property.value.size / sizeof(uint32_t)
                );
            break;
        case serialization::PropertyType::integer64:
            archive_object[name] = std::vector<uint64_t>(
                static_cast<uint64_t*>(property.value.data),
                static_cast<uint64_t*>(property.value.data) + property.value.size / sizeof(uint64_t)
                );
            break;
        case serialization::PropertyType::floating32:
            archive_object[name] = std::vector<float>(
                static_cast<float*>(property.value.data),
                static_cast<float*>(property.value.data) + property.value.size / sizeof(float)
                );
            break;
        case serialization::PropertyType::floating64:
            archive_object[name] = std::vector<double>(
                static_cast<double*>(property.value.data),
                static_cast<double*>(property.value.data) + property.value.size / sizeof(double)
                );
            break;
        case serialization::PropertyType::character:
            archive_object[name] = std::vector<char>(
                static_cast<char*>(property.value.data),
                static_cast<char*>(property.value.data) + property.value.size / sizeof(char)
                );
            break;
        case serialization::PropertyType::binary:
            archive_object[name] = std::vector<uint8_t>(
                static_cast<uint8_t*>(property.value.data),
                static_cast<uint8_t*>(property.value.data) + property.value.size / sizeof(uint8_t)
                );
            break;
        case serialization::PropertyType::integer128:
        case serialization::PropertyType::invalid:
            LOG_ERROR_TAG("Json Archiver", "Invalid property type for array in property {}", name);
            break;
        }
        break;
    }
    case serialization::PropertyContainerType::string:
    {
        archive_object[name] = std::string(static_cast<const char*>(property.value.data), property.elements_number);
        break;
    }
    case serialization::PropertyContainerType::null_term_string:
    {
        archive_object[name] = std::string(static_cast<const char*>(property.value.data));
        break;
    }
    case serialization::PropertyContainerType::vec1:
        LOG_ERROR_TAG("Json Archiver", "Cannot archive vec1 to json");
        break;
    case serialization::PropertyContainerType::vec2:
        LOG_ERROR_TAG("Json Archiver", "Cannot archive vec2 to json");
        break;
    case serialization::PropertyContainerType::vec3:
        LOG_ERROR_TAG("Json Archiver", "Cannot archive vec3 to json");
        break;
    case serialization::PropertyContainerType::vec4:
        LOG_ERROR_TAG("Json Archiver", "Cannot archive vec4 to json");
        break;
    }
}

JsonDearchiver::JsonDearchiver(std::istream& input):
    input(input) {}

void JsonDearchiver::dearchive()
{
    archive_object = nlohmann::json::parse(input);
}

bool JsonDearchiver::get_property(const std::string& name, serialization::Property& out)
{
    if (!archive_object.contains(name))
    {
        LOG_ERROR_TAG("Json Dearchiver", "Property {} not found in JSON", name);
        return false;
    }

    const auto& json_value = archive_object[name];
    const auto value = json_value.get_binary();
    size_t property_size = 0;
    serialization::PropertyContainerType container_type{};
    size_t elements_number = 0;


    if (json_value.is_number_integer())
    {
        container_type = serialization::PropertyContainerType::scalar;
        elements_number = 1;
        switch (out.type)
        {
        case serialization::PropertyType::integer8:
            property_size = sizeof(uint8_t);
            break;
        case serialization::PropertyType::integer16:
            property_size = sizeof(uint16_t);
            break;
        case serialization::PropertyType::integer32:
            property_size = sizeof(uint32_t);
            break;
        case serialization::PropertyType::integer64:
            property_size = sizeof(uint64_t);
            break;
        default:
            LOG_ERROR_TAG("Json Dearchiver", "Type mismatch for property {}", name);
            return false;
        }
    }
    else if (json_value.is_number_float())
    {
        container_type = serialization::PropertyContainerType::scalar;
        elements_number = 1;

        switch (out.type)
        {
        case serialization::PropertyType::floating32:
            property_size = sizeof(float);
            break;
        case serialization::PropertyType::floating64:
            property_size = sizeof(double);
            break;
        default:
            LOG_ERROR_TAG("Json Dearchiver", "Type mismatch for property {}", name);
            return false;
        }
    }
    else if (json_value.is_string())
    {
        if (out.type != serialization::PropertyType::character)
        {
            LOG_ERROR_TAG("Json Dearchiver", "Container type mismatch for string property {}", name);
            return false;
        }

        container_type = serialization::PropertyContainerType::null_term_string;
        elements_number = json_value.get<std::string>().size() + 1;
        property_size = json_value.get<std::string>().size() + 1;
    }
    else if (json_value.is_array())
    {
        const auto& array = json_value;

        out.container_type = serialization::PropertyContainerType::array;
        elements_number = array.size();
        if (array.empty())
        {
            out.value = Buffer{nullptr, 0};
            return true;
        }

        size_t element_size = 0;

        switch (out.type)
        {
        case serialization::PropertyType::integer8:
            element_size = sizeof(uint8_t);
            break;
        case serialization::PropertyType::integer16:
            element_size = sizeof(uint16_t);
            break;
        case serialization::PropertyType::integer32:
            element_size = sizeof(uint32_t);
            break;
        case serialization::PropertyType::integer64:
            element_size = sizeof(uint64_t);
            break;
        case serialization::PropertyType::floating32:
            element_size = sizeof(float);
            break;
        case serialization::PropertyType::floating64:
            element_size = sizeof(double);
            break;
        case serialization::PropertyType::character:
            element_size = sizeof(char);
            break;
        default:
            break;
        }
        property_size = element_size * elements_number;
    }
    else
    {
        LOG_ERROR_TAG("Json Dearchiver", "Unsupported JSON type for property {}", name);
        return false;
    }


    // out.value = Buffer::copy(value.data, property_size);
    out.container_type = container_type;
    out.elements_number = elements_number;
    return true;
}

} // portal
