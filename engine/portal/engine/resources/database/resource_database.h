//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <expected>
#include <llvm/ADT/SmallVector.h>

#include "../source/resource_source.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/serialization/archive.h"


namespace portal
{

struct SourceMetadata
{
    // Resource Information
    StringId resource_id = INVALID_STRING_ID;
    ResourceHandle handle = INVALID_RESOURCE_HANDLE;
    ResourceType type = ResourceType::Unknown;
    llvm::SmallVector<ResourceHandle> dependencies{};

    // Source Information
    StringId source = INVALID_STRING_ID;
    SourceFormat format = SourceFormat::Unknown;

    void archive(ArchiveObject& archive) const;
    static SourceMetadata dearchive(ArchiveObject& archive);
};

enum class DatabaseErrorBit: uint8_t
{
    Success         = 0b00000000,
    NotFound        = 0b00000001,
    Conflict        = 0b00000010,
    MissingResource = 0b00000100,
    StaleMetadata   = 0b00001000,
    MissingMetadata = 0b00010000,

    Unspecified = std::numeric_limits<uint8_t>::max(),
};

using DatabaseError = Flags<DatabaseErrorBit>;

class ResourceDatabase
{
public:
    virtual ~ResourceDatabase() = default;

    virtual std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) = 0;
    virtual std::expected<SourceMetadata, DatabaseError> find(ResourceHandle handle) = 0;

    virtual DatabaseError add(SourceMetadata meta) = 0;

    virtual DatabaseError remove(StringId resource_id) = 0;
    virtual DatabaseError remove(ResourceHandle resource_id) = 0;

    virtual std::unique_ptr<resources::ResourceSource> create_source(SourceMetadata meta) = 0;
};

} // portal
