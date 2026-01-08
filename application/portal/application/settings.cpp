//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "portal/application/settings.h"

#include <fmt/ranges.h>
#include <fmt/std.h>

#include "portal/core/files/file_system.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{
static std::unique_ptr<Settings> g_settings;

constexpr auto LOG_LEVEL_ENTRY = "log-level";

void Settings::init(const SettingsArchiveType type, const std::filesystem::path& settings_file_name)
{
    if (g_settings)
    {
        throw std::runtime_error("Settings already initialized");
    }

    if (!FileSystem::exists(settings_file_name))
    {
        throw std::runtime_error("Local settings file does not exist");
    }

    const auto local_settings_path = std::filesystem::current_path() / "settings.json";
    const auto mutable_settings_path = FileSystem::get_data_home() / settings_file_name;

    g_settings =  std::unique_ptr<Settings>(new Settings(type, local_settings_path));
    if (FileSystem::exists(mutable_settings_path))
    {
        auto user_settings = Settings(type, mutable_settings_path);
        g_settings->update(user_settings);
    }

    auto log_level_string = g_settings->get_setting<std::string>(LOG_LEVEL_ENTRY);
    if (log_level_string)
    {
        const auto log_level = portal::from_string<Log::LogLevel>(*log_level_string);
        Log::set_default_log_level(log_level);
    }

    get().debug_print();
}

void Settings::shutdown()
{
    g_settings.reset();
}

Settings& Settings::get()
{
    if (!g_settings)
    {
        throw std::runtime_error("Settings not initialized");
    }
    return *g_settings;
}

Settings::Settings(const SettingsArchiveType type, const std::filesystem::path& path) : type(type), settings_path(std::filesystem::absolute(path))
{
    switch (type)
    {
    case SettingsArchiveType::Json:
        root = std::make_unique<JsonArchive>();
        break;
    }

    load();
}

Settings::~Settings()
{
    dump();
}

void Settings::load()
{
    switch (type)
    {
    case SettingsArchiveType::Json:
        {
            auto& archive = dynamic_cast<JsonArchive&>(*root);
            archive.read(settings_path);
            update(archive);
            break;
        }
    }
}

void Settings::dump() const
{
    switch (type)
    {
    case SettingsArchiveType::Json:
        {
            auto& archive = dynamic_cast<JsonArchive&>(*root);
            archive.update(*this);
            archive.dump(settings_path);
            break;
        }
    }
}

void Settings::debug_print() const
{
    debug_print("", *this);
}

void Settings::debug_print_scalar(const std::string& name, const reflection::Property& prop) const
{
    switch (prop.type)
    {
    case reflection::PropertyType::binary:
        {
            std::byte data = *prop.value.as<std::byte*>();
            LOG_DEBUG("{}: {}", name, static_cast<uint8_t>(data));
            break;
        }
    case reflection::PropertyType::integer8:
        {
            uint8_t data = *prop.value.as<uint8_t*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::integer16:
        {
            uint16_t data = *prop.value.as<uint16_t*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::integer32:
        {
            uint32_t data = *prop.value.as<uint32_t*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::integer64:
        {
            uint64_t data = *prop.value.as<uint64_t*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::integer128:
        {
            uint128_t data = *prop.value.as<uint128_t*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::floating32:
        {
            float data = *prop.value.as<float*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::floating64:
        {
            double data = *prop.value.as<double*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::character:
        {
            double data = *prop.value.as<double*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::boolean:
        {
            bool data = *prop.value.as<bool*>();
            LOG_DEBUG("{}: {}", name, data);
            break;;
        }
    case reflection::PropertyType::invalid:
        LOG_DEBUG("{}: null", name);
        break;
    case reflection::PropertyType::object:
    case reflection::PropertyType::null_term_string:
    case reflection::PropertyType::string:
        LOG_WARN("{}: not a scalarn", name);
        break;
    }
}


void Settings::debug_print_array(const std::string& name, const reflection::Property& prop) const
{
    switch (prop.type)
    {
    case reflection::PropertyType::binary:
    case reflection::PropertyType::character:
        {
            llvm::SmallVector<char> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<char>, char>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::integer8:
        {
            llvm::SmallVector<uint8_t> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<uint8_t>, uint8_t>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::integer16:
        {
            llvm::SmallVector<uint16_t> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<uint16_t>, uint16_t>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::integer32:
        {
            llvm::SmallVector<uint32_t> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<uint32_t>, uint32_t>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::integer64:
        {
            llvm::SmallVector<uint64_t> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<uint64_t>, uint64_t>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::integer128:
        {
            llvm::SmallVector<uint128_t> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<uint128_t>, uint128_t>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::floating32:
        {
            llvm::SmallVector<float> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<float>, float>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::floating64:
        {
            llvm::SmallVector<double> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<double>, double>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::boolean:
        {
            llvm::SmallVector<bool> buffer;
            buffer.resize(prop.elements_number);
            format_array<llvm::SmallVector<bool>, bool>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }

    case reflection::PropertyType::null_term_string:
    case reflection::PropertyType::string:
        {
            std::vector<std::string> buffer;
            buffer.resize(prop.elements_number);
            format_array<std::vector<std::string>, std::string>(name, prop, buffer);
            LOG_DEBUG("{}: [{}]", name, fmt::join(buffer, ", "));
            break;
        }
    case reflection::PropertyType::object:
        {
            auto* objects = prop.value.as<ArchiveObject*>();
            for (size_t i = 0; i < prop.elements_number; ++i)
            {
                std::string element_name = fmt::format("{}[{}]", name, i);
                debug_print(element_name, objects[i]);
            }
            break;
        }
    case reflection::PropertyType::invalid:
        break;
    }
}

void Settings::debug_print(std::string base_name, const ArchiveObject& object) const
{
    for (auto& [key, prop] : object)
    {
        std::string name;
        if (base_name.empty())
            name = key;
        else
            name = fmt::format("{}.{}", base_name, std::string_view(key));

        switch (prop.container_type)
        {
        case reflection::PropertyContainerType::invalid:
            LOG_DEBUG("{}: null", name);
            break;
        case reflection::PropertyContainerType::scalar:
            debug_print_scalar(name, prop);
            break;
        case reflection::PropertyContainerType::null_term_string:
            {
                const size_t string_length = prop.elements_number - 1;;
                const auto* data = prop.value.as<const char*>();
                LOG_DEBUG("{}: {}", name, std::string(data, string_length));
                break;
            }
        case reflection::PropertyContainerType::string:
            {
                const size_t string_length = prop.elements_number;
                const auto* data = prop.value.as<const char*>();
                LOG_DEBUG("{}: {}", name, std::string(data, string_length));
                break;
            }
        case reflection::PropertyContainerType::object:
            {
                debug_print(name, *prop.value.as<ArchiveObject*>());
                break;
            }
        case reflection::PropertyContainerType::array:
            {
                debug_print_array(name, prop);
                break;
            }
        case reflection::PropertyContainerType::vector:
            break;
        case reflection::PropertyContainerType::matrix:
            break;
        }
    }
}

reflection::Property& Settings::get_property_from_map(const PropertyName name)
{
    std::lock_guard lock_guard(lock);
    return ArchiveObject::get_property_from_map(name);
}

reflection::Property& Settings::add_property_to_map(const PropertyName name, reflection::Property&& property)
{
    std::lock_guard lock_guard(lock);
    return ArchiveObject::add_property_to_map(name, std::move(property));
}
} // portal
