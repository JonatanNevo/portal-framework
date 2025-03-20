//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include <memory>
#include <string>

#include "portal/core/assert.h"
#include "portal/serialization/archive.h"
#include "portal/serialization/impl/memory_archive.h"

namespace portal
{
class Configuration
{
public:
    Configuration()
    {
        // TODO: support other formats
        archiver = std::make_shared<MemoryArchive>();
        dearchiver = std::dynamic_pointer_cast<Dearchiver>(archiver);
    }

    template <typename T> requires std::is_pointer_v<T>
    T get_with_default(const std::string& name, T default_value) const
    {
        return reinterpret_cast<T>(get_with_default(name, reinterpret_cast<uintptr_t>(default_value)));
    }

    template <typename T>
    T get_with_default(const std::string& name, T default_value) const
    {
        T output;
        const bool success = get<T>(name, output);
        if (!success)
            return default_value;
        return output;
    }

    template <typename T>
    bool get(const std::string& name, T& value) const
    {
        return dearchiver->get_property(name, value);
    }

    template <typename T> requires std::is_pointer_v<T>
    void set(const std::string& name, T value)
    {
        auto ptr_value = reinterpret_cast<uintptr_t>(value);
        archiver->add_property(name, ptr_value);
    }

    template <typename T> requires (!std::is_pointer_v<T>)
    void set(const std::string& name, T value)
    {
        archiver->add_property(name, value);
    }

protected:
    std::shared_ptr<Archiver> archiver;
    std::shared_ptr<Dearchiver> dearchiver;
};
}


