//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "json_archive.h"

#include <fstream>

#include "portal/core/files/file_system.h"

namespace portal
{

void JsonArchive::archive(const std::filesystem::path& output_path)
{
    if (!FileSystem::exists(output_path.parent_path()))
    {
        LOG_ERROR_TAG("Json Archive", "Output directory {} does not exist", output_path.parent_path().string());
        return;
    }

    std::ofstream output(output_path);
    if (!output.is_open())
    {
        LOG_ERROR_TAG("Json Archive", "Failed to open output file {}", output_path.string());
        return;
    }

    archive(output);
}

void JsonArchive::archive(std::ostream& output)
{
    output << prepare_json();
}

void JsonArchive::read(const std::filesystem::path& input_path)
{
    if (!FileSystem::exists(input_path))
    {
        LOG_ERROR_TAG("Json Archive", "Input file {} does not exist", input_path.string());
        return;
    }

    std::ifstream input(input_path);
    if (!input.is_open())
    {
        LOG_ERROR_TAG("Json Archive", "Failed to open input file {}", input_path.string());
        return;
    }
    read(input);
}

void JsonArchive::read(std::istream& input)
{
    deserialize(nlohmann::json::parse(input, nullptr, false, true));
}

nlohmann::json JsonArchive::prepare_json()
{
    return prepare_object(this);
}

nlohmann::json JsonArchive::prepare_object(ArchiveObject* object)
{
    nlohmann::json archive_object({});
    for (const auto& [key, prop] : object->property_map)
    {
        switch (prop.container_type)
        {
        case serialize::PropertyContainerType::object:
            PORTAL_ASSERT(prop.type == serialize::PropertyType::object, "Object property type must be object");
            archive_object[key] = prepare_object(prop.value.as<ArchiveObject*>());
            break;
        case serialize::PropertyContainerType::scalar:
            switch (prop.type)
            {
            case serialize::PropertyType::integer8:
                archive_object[key] = *prop.value.as<uint8_t*>();
                break;
            case serialize::PropertyType::integer16:
                archive_object[key] = *prop.value.as<uint16_t*>();
                break;
            case serialize::PropertyType::integer32:
                archive_object[key] = *prop.value.as<uint32_t*>();
                break;
            case serialize::PropertyType::integer64:
                archive_object[key] = *prop.value.as<uint64_t*>();
                break;
            case serialize::PropertyType::integer128:
                LOG_ERROR_TAG("Json Archiver", "Cannot archive integer128 to json");
                break;
            case serialize::PropertyType::floating32:
                archive_object[key] = *prop.value.as<float*>();
                break;
            case serialize::PropertyType::floating64:
                archive_object[key] = *prop.value.as<double*>();
                break;
            case serialize::PropertyType::character:
                archive_object[key] = *prop.value.as<char*>();
                break;
            case serialize::PropertyType::boolean:
                archive_object[key] = *prop.value.as<bool*>();
                break;
            case serialize::PropertyType::binary:
            case serialize::PropertyType::invalid:
            case serialize::PropertyType::object:
            case serialize::PropertyType::null_term_string:
            case serialize::PropertyType::string:
                LOG_ERROR_TAG("Json Archiver", "Invalid property type for scalar in property {}", std::string_view(key));
                break;
            }
            break;
        case serialize::PropertyContainerType::array:
            switch (prop.type)
            {
            case serialize::PropertyType::integer8:
                extract_array_elements<uint8_t>(archive_object, prop, key);
                break;
            case serialize::PropertyType::integer16:
                extract_array_elements<uint16_t>(archive_object, prop, key);
                break;
            case serialize::PropertyType::integer32:
                extract_array_elements<uint32_t>(archive_object, prop, key);
                break;
            case serialize::PropertyType::integer64:
                extract_array_elements<uint64_t>(archive_object, prop, key);
                break;
            case serialize::PropertyType::floating32:
                extract_array_elements<float>(archive_object, prop, key);
                break;
            case serialize::PropertyType::floating64:
                extract_array_elements<double>(archive_object, prop, key);
                break;
            case serialize::PropertyType::character:
                extract_array_elements<char>(archive_object, prop, key);
                break;
            case serialize::PropertyType::binary:
                extract_array_elements<byte>(archive_object, prop, key);
                break;
            case serialize::PropertyType::boolean:
                extract_array_elements<bool>(archive_object, prop, key);
                break;
            case serialize::PropertyType::null_term_string:
                extract_array_elements<std::string>(archive_object, prop, key, 1);
                break;
            case serialize::PropertyType::string:
            {
                extract_array_elements<std::string>(archive_object, prop, key, 0);
                break;
            }
            case serialize::PropertyType::object:
            {
                std::vector<nlohmann::json> array_elements;
                for (int i = 0; i < prop.elements_number; i++)
                {
                    array_elements.emplace_back(prepare_object(prop.value.as<ArchiveObject*>() + i));
                }
                archive_object[key] = array_elements;
                break;
            }
            case serialize::PropertyType::integer128:
            case serialize::PropertyType::invalid:
                LOG_ERROR_TAG("Json Archiver", "Invalid property type for array in property {}", std::string_view(key));
                break;
            }
            break;
        case serialize::PropertyContainerType::string:
            archive_object[key] = std::string(prop.value.as<const char*>(), prop.elements_number);
            break;
        case serialize::PropertyContainerType::null_term_string:
            archive_object[key] = std::string(prop.value.as<const char*>());
            break;
        case serialize::PropertyContainerType::vec1:
            LOG_ERROR_TAG("Json Archiver", "Cannot archive vec1 to json");
            break;
        case serialize::PropertyContainerType::vec2:
            LOG_ERROR_TAG("Json Archiver", "Cannot archive vec2 to json");
            break;
        case serialize::PropertyContainerType::vec3:
            LOG_ERROR_TAG("Json Archiver", "Cannot archive vec3 to json");
            break;
        case serialize::PropertyContainerType::vec4:
            LOG_ERROR_TAG("Json Archiver", "Cannot archive vec4 to json");
            break;
        }
    }
    return archive_object;
}

void JsonArchive::deserialize(const nlohmann::json& input)
{
    deserialize_object(this, input);
}

void JsonArchive::deserialize_object(ArchiveObject* root, const nlohmann::json& input)
{
    for (auto& [key, value] : input.items())
    {
        switch (value.type())
        {
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
        {
            auto* child = root->create_child(key);
            deserialize_object(child, value);
            break;
        }
        case nlohmann::detail::value_t::number_integer:
            root->add_property<nlohmann::json::number_integer_t>(key, value);
            break;
        case nlohmann::detail::value_t::number_unsigned:
            root->add_property<nlohmann::json::number_unsigned_t>(key, value);
            break;
        case nlohmann::detail::value_t::array:
            deserialize_array(root, key, value);
            break;
        case nlohmann::detail::value_t::string:
            root->add_property<nlohmann::json::string_t>(key, value);
            break;
        case nlohmann::detail::value_t::boolean:
            root->add_property<nlohmann::json::boolean_t>(key, value);
            break;
        case nlohmann::detail::value_t::number_float:
            root->add_property<nlohmann::json::number_float_t>(key, value);
            break;
        case nlohmann::detail::value_t::binary:
            root->add_binary_block(key, value);
            break;
        case nlohmann::detail::value_t::discarded:
            break;
        }
    }
}

void JsonArchive::deserialize_array(ArchiveObject* root, const std::string& key, const nlohmann::json& array)
{
    if (array.empty())
    {
        add_property_to_map(
            key,
            {
                {},
                serialize::PropertyType::invalid,
                serialize::PropertyContainerType::array,
                0
            }
            );
        return;
    }

    const auto array_value_type = array[0].type();
    switch (array_value_type)
    {
    case nlohmann::detail::value_t::object:
    {
        std::vector<ArchiveObject> objects;
        for (auto& value : array)
        {
            auto& child = objects.emplace_back();
            deserialize_object(&child, value);
        }

        const auto buffer = Buffer::allocate(objects.size() * sizeof(ArchiveObject));
        for (int i = 0; i < objects.size(); ++i)
        {
            new(buffer.as<ArchiveObject*>() + i) ArchiveObject(std::move(objects[i]));
        }

        root->add_property_to_map(
            key,
            {
                std::move(buffer),
                serialize::PropertyType::object,
                serialize::PropertyContainerType::array,
                objects.size()
            }
            );
        break;
    }
    case nlohmann::detail::value_t::string:
    {
        std::vector<std::string> string_array;
        for (const auto& value : array)
        {
            string_array.emplace_back(value.get<std::string>());
        }
        root->add_property(key, string_array);
        break;
    }
    case nlohmann::detail::value_t::boolean:
    {
        std::vector<typename nlohmann::json::number_integer_t> int_array;
        for (const auto& value : array)
        {
            int_array.emplace_back(value.get<nlohmann::json::number_integer_t>());
        }
        root->add_property(key, int_array);
        break;
    }
    case nlohmann::detail::value_t::number_integer:
    {
        std::vector<typename nlohmann::json::number_integer_t> int_array;
        for (const auto& value : array)
        {
            int_array.emplace_back(value.get<nlohmann::json::number_integer_t>());
        }
        root->add_property(key, int_array);
        break;
    }
    case nlohmann::detail::value_t::number_unsigned:
    {
        std::vector<typename nlohmann::json::number_unsigned_t> uint_array;
        for (const auto& value : array)
        {
            uint_array.emplace_back(value.get<nlohmann::json::number_unsigned_t>());
        }
        root->add_property(key, uint_array);
        break;
    }
    case nlohmann::detail::value_t::number_float:
    {
        std::vector<typename nlohmann::json::number_float_t> float_array;
        for (const auto& value : array)
        {
            float_array.emplace_back(value.get<nlohmann::json::number_float_t>());
        }
        root->add_property(key, float_array);
        break;
    }
    case nlohmann::detail::value_t::array:
        LOG_ERROR_TAG("Json Archiver", "Cannot deserialize array of arrays from json");
        break;
    case nlohmann::detail::value_t::binary:
    case nlohmann::detail::value_t::null:
    case nlohmann::detail::value_t::discarded:
        break;
    };
}
} // portal
