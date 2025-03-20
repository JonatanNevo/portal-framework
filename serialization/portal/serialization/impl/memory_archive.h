//
// Created by Jonatan Nevo on 01/03/2025.
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
