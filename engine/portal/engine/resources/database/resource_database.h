//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <expected>
#include <llvm/ADT/SmallVector.h>

#include "portal/application/modules/module.h"
#include "portal/engine/renderer/image/image_types.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/serialization/archive.h"


namespace portal
{
namespace resources
{
    class ResourceSource;
}

struct SourceMetadata;

struct EmptyMeta
{
    void archive(ArchiveObject&) const {}
};

struct CompositeMetadata
{
    std::unordered_map<std::string, SourceMetadata> children;
    std::string type;

    void archive(ArchiveObject& archive) const;
    static CompositeMetadata dearchive(ArchiveObject& archive);
};

struct TextureMetadata
{
    bool hdr;
    size_t width;
    size_t height;
    renderer::ImageFormat format;

    void archive(ArchiveObject& archive) const;
    static TextureMetadata dearchive(ArchiveObject& archive);
};

struct MaterialMetadata
{
    StringId shader;

    void archive(ArchiveObject& archive) const;
    static MaterialMetadata dearchive(ArchiveObject& archive);
};

struct SourceMetadata
{
    // Resource Information
    StringId resource_id = INVALID_STRING_ID;
    ResourceType type = ResourceType::Unknown;
    llvm::SmallVector<StringId> dependencies{};

    // Source Information
    StringId source = INVALID_STRING_ID;
    SourceFormat format = SourceFormat::Unknown;

    // For internal use only
    StringId full_source_path = INVALID_STRING_ID;

    // Specific metadata
    std::variant<TextureMetadata, CompositeMetadata, MaterialMetadata, EmptyMeta> meta = EmptyMeta{};

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
    CorruptMetadata = 0b00100000,
    DatabaseMissing = 0b01000000,


    Unspecified = std::numeric_limits<uint8_t>::max(),
};

using DatabaseError = Flags<DatabaseErrorBit>;

class ResourceDatabase : public Module<>
{
public:
    explicit ResourceDatabase(ModuleStack& stack, const StringId& name) : Module<>(stack, name) {}
    ~ResourceDatabase() override = default;

    virtual std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) = 0;
    virtual DatabaseError add(StringId resource_id, SourceMetadata meta) = 0;
    virtual DatabaseError remove(StringId resource_id) = 0;

    virtual std::unique_ptr<resources::ResourceSource> create_source(StringId resource_id, SourceMetadata meta) = 0;
};
} // portal
