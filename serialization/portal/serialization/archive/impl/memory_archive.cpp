//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/serialization/archive/impl/memory_archive.h"

namespace portal
{

void MemoryArchive::archive()
{
    LOG_WARN_TAG("MemoryArchive", "There is no implementation for MemoryArchive::archive()");
}

void MemoryArchive::load()
{
    LOG_WARN_TAG("MemoryArchive", "There is no implementation for MemoryArchive::dearchive()");
}

void MemoryArchive::add_property(const std::string& name, const archiving::Property& property)
{
    properties[name] = property;
}

bool MemoryArchive::get_property(const std::string& name, archiving::Property& out)
{
    if (!properties.contains(name))
        return false;

    out = properties[name];
    return true;
}
} // portal