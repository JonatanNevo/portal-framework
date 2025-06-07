//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/serialization/archive.h"

namespace portal
{
class MemoryArchive final : public Archiver, public Dearchiver
{
public:
    void archive() override;
    void dearchive() override;

protected:
    void add_property(const std::string& name, const serialization::Property& property) override;
    bool get_property(const std::string& name, serialization::Property& out) override;

    std::map<std::string, serialization::Property> properties;
};
} // portal
