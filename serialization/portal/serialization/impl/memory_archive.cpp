//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "memory_archive.h"

namespace portal
{

void MemoryArchive::archive()
{
    LOG_CORE_WARN_TAG("MemoryArchive", "There is no implementation for MemoryArchive::archive()");
}

void MemoryArchive::dearchive()
{
    LOG_CORE_WARN_TAG("MemoryArchive", "There is no implementation for MemoryArchive::dearchive()");
}

void MemoryArchive::add_property(const std::string& name, const serialization::Property& property)
{
    properties[name] = property;
}

bool MemoryArchive::get_property(const std::string& name, serialization::Property& out)
{
    if (!properties.contains(name))
        return false;

    out = properties[name];
    return true;
}
} // portal