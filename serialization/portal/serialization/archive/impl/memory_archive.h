//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/serialization/archive/archive.h"

namespace portal
{
class MemoryArchive final : public Archiver, public Dearchiver
{
public:
    void archive() override;
    void load() override;

protected:
    void add_property(const std::string& name, const archiving::Property& property) override;
    bool get_property(const std::string& name, archiving::Property& out) override;

    std::map<std::string, archiving::Property> properties;
};
} // portal
