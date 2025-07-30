//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <nlohmann/json.hpp>
#include "portal/serialization/archive.h"

namespace portal
{
class JsonArchive final : public ArchiveObject
{
public:
    void dump(const std::filesystem::path& output_path);
    void dump(std::ostream& output);

    void read(const std::filesystem::path& input_path);
    void read(std::istream& input);

protected:
    nlohmann::json prepare_json();
    static nlohmann::json prepare_object(ArchiveObject* object);

    void deserialize(const nlohmann::json& input);
    void deserialize_object(ArchiveObject* root, const nlohmann::json& input);
    void deserialize_array(ArchiveObject* root, const std::string& key,const nlohmann::json& array);

private:
    template<typename T>
    static void extract_array_elements(nlohmann::json& archive, const serialize::Property& prop, const std::string& key, int element_number_skew = 1)
    {
        std::vector<T> array_elements;
        array_elements.reserve(prop.elements_number);

        for (size_t i = 0; i < prop.elements_number; i++)
        {
            auto& property_value = prop.value.as<ArchiveObject*>()[i].property_map["v"];
            if constexpr (serialize::String<T>)
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
} // portal
