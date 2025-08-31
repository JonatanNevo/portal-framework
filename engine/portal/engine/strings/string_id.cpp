//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_id.h"

#include "portal/core/log.h"
#include "portal/engine/strings/string_registry.h"
#include "portal/serialization/serialize.h"

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

// TODO: when serializing string ids, we should save all string in some `string map` and serialize only the ids

void StringId::serialize(Serializer& s) const
{
    s.add_value(string);
    s.add_value(id);
}

StringId StringId::deserialize(Deserializer& d)
{
    std::string string;
    uint64_t id;

    d.get_value(string);
    d.get_value(id);

    return StringId{id, string};
}

} // portal
