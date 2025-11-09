//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_id.h"

#include "portal/core/log.h"
#include "portal/core/strings/string_registry.h"

namespace portal
{

StringId::StringId(const uint64_t id): id(id)
{
    string = StringRegistry::find(id);
    if (string == INVALID_STRING_VIEW)
    {
        LOG_ERROR_TAG("StringId", "StringId with id {} not found in registry", id);
    }
}

StringId::StringId(const uint64_t id, const std::string_view string): id(id)
{
    this->string = StringRegistry::store(id, string);
}

StringId::StringId(const uint64_t id, const std::string& string): StringId(id, std::string_view(string)) {}

bool StringId::operator==(const StringId& other) const
{
    return id == other.id;
}

} // portal
