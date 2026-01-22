//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>
#include <ranges>

#include "portal/core/log.h"
#include "portal/core/concurrency/reentrant_spin_lock.h"
#include "portal/serialization/archive.h"

namespace portal
{
enum class SettingsArchiveType
{
    Json
};

/**
 * A settings singleton class.
 * To call, the static method `init` needs to be called once, from then, call the static `get` to retrive an instance
 */
class ProjectSettings final : public ArchiveObject
{
public:
    static ProjectSettings create_settings(SettingsArchiveType type, const std::filesystem::path& working_directory, const std::filesystem::path& settings_file_name);

    ~ProjectSettings() override;

    ProjectSettings(const ProjectSettings&) = delete;
    ProjectSettings& operator=(const ProjectSettings&) = delete;

    ProjectSettings(ProjectSettings&& other) noexcept;
    ProjectSettings& operator=(ProjectSettings&& other) noexcept;

    void load();
    void dump() const;

    template <typename T>
    T get_setting(const PropertyName& name, const T& default_value)
    {
        auto optional_t = get_setting<T>(name);
        if (!optional_t.has_value())
        {
            set_setting(name, default_value);
            return default_value;
        }
        return *optional_t;
    }

    template <typename T>
    std::optional<T> get_setting(const PropertyName& name) const
    {
        auto split_view = name | std::ranges::views::split('.');
        auto split_size = std::ranges::distance(split_view);

        ArchiveObject* current_node = root.get();
        for (auto part : split_view)
        {
            std::string_view part_str{part};


            if (--split_size == 0)
            {
                T out;
                if (current_node->get_property(part_str, out))
                    return out;
                return std::nullopt;
            }

            current_node = current_node->get_object(part_str);
            if (!current_node)
            {
                LOG_ERROR_TAG("Settings", "Failed to get setting \"{}\"", name);
                return std::nullopt;
            }
        }

        return std::nullopt;
    }
    
    template <typename T>
    void set_setting(const PropertyName& name, const T& value)
    {
        auto split_view = name | std::ranges::views::split('.');
        auto split_size = std::ranges::distance(split_view);

        ArchiveObject* current_node = root.get();
        for (auto part : split_view)
        {
            std::string_view part_str{part};

            if (--split_size == 0)
            {
                current_node->add_property(part_str, value);
                return;
            }

            auto* next_node = current_node->get_object(part_str);
            if (!next_node)
            {
                next_node = current_node->create_child(part_str);
            }
            current_node = next_node;
        }
    }

    void debug_print() const;

private:
    void debug_print(std::string base_name, const ArchiveObject& object) const;
    void debug_print_scalar(const std::string& name, const reflection::Property& prop) const;
    void debug_print_array(const std::string& name, const reflection::Property& prop) const;

    explicit ProjectSettings(SettingsArchiveType type, const std::filesystem::path& path);


protected:
    reflection::Property& get_property_from_map(PropertyName name) override;
    reflection::Property& add_property_to_map(PropertyName name, reflection::Property&& property) override;

private:
    ReentrantSpinLock<> lock;
    SettingsArchiveType type;

    std::filesystem::path settings_path;
    // Memory ownership of properties
    std::unique_ptr<ArchiveObject> root;
};
} // portal
