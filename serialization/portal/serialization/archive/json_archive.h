//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <nlohmann/json.hpp>
#include "portal/serialization/archive.h"

namespace portal
{

/**
 * @brief JSON format implementation of ArchiveObject for human-readable serialization.
 *
 * JsonArchive provides JSON serialization/deserialization by converting the intermediate
 * ArchiveObject property tree into nlohmann::json format. This enables human-readable,
 * editable configuration files, saved games, resource metadata, and data exchange with
 * external tools.
 *
 * ## Usage Example
 *
 * @code
 * // Serialization
 * JsonArchive archive;
 * config.archive(archive);  // User type populates ArchiveObject
 * archive.dump("settings.json", 4);  // Convert to JSON and write
 *
 * // Deserialization
 * JsonArchive loaded;
 * loaded.read("settings.json");  // Parse JSON into ArchiveObject
 * Config restored = Config::dearchive(loaded);  // User type extracts data
 * @endcode
 *
 * @see ArchiveObject for the underlying property container
 * @see Archiveable concept for making types serializable
 */
class JsonArchive final : public ArchiveObject
{
public:
    /**
     * @brief Serializes the ArchiveObject property tree to a JSON file.
     *
     * Converts the internal ArchiveObject representation to nlohmann::json format and
     * writes it to the specified file. File I/O errors are logged but don't throw exceptions.
     *
     * @param output_path Path to the output JSON file (created/overwritten)
     * @param indent Number of spaces for indentation (4 = pretty-printed, 0 = compact)
     */
    void dump(const std::filesystem::path& output_path, size_t indent = 4);

    /**
     * @brief Serializes the ArchiveObject property tree to an output stream in JSON format.
     *
     * Converts the internal ArchiveObject representation to nlohmann::json and writes it
     * to the provided stream with the specified indentation.
     *
     * @param output The output stream to write JSON data to
     * @param indent Number of spaces for indentation (4 = pretty-printed, 0 = compact)
     */
    void dump(std::ostream& output, size_t indent = 4);

    /**
     * @brief Deserializes JSON content from a file into this ArchiveObject.
     *
     * Parses the JSON file using nlohmann::json and populates this ArchiveObject's property
     * map with the parsed data. Parse errors and file I/O errors are logged to "Json Archive" tag.
     *
     * @param input_path Path to the input JSON file to read and parse
     */
    void read(const std::filesystem::path& input_path);

    /**
     * @brief Deserializes JSON content from an input stream into this ArchiveObject.
     *
     * Parses JSON from the stream and populates this ArchiveObject's property map.
     * Parse errors are logged but don't throw exceptions.
     *
     * @param input The input stream containing JSON data to parse
     */
    void read(std::istream& input);

protected:
    nlohmann::json prepare_json();
    static nlohmann::json prepare_object(ArchiveObject* object);

    void deserialize(const nlohmann::json& input);
    void deserialize_object(ArchiveObject* root, const nlohmann::json& input);
    void deserialize_array(ArchiveObject* root, const std::string& key, const nlohmann::json& array);

private:
    template <typename T>
    static void extract_array_elements(nlohmann::json& archive, const reflection::Property& prop, const auto& key, int element_number_skew = 1)
    {
        std::vector<T> array_elements;
        array_elements.reserve(prop.elements_number);

        for (size_t i = 0; i < prop.elements_number; i++)
        {
            auto& property_value = prop.value.as<ArchiveObject*>()[i].property_map["v"];
            if constexpr (reflection::String<T>)
            {
                array_elements.emplace_back(property_value.value.as<const char*>(), property_value.elements_number - element_number_skew);
            }
            else
            {
                T& value = *property_value.value.as<T*>();
                array_elements.emplace_back(value);
            }
        }
        archive[key] = array_elements;
    };
};
} // namespace portal
