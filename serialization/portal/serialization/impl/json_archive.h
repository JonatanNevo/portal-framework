//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <nlohmann/json.hpp>
#include "portal/serialization/archive.h"

namespace portal
{
class JsonArchiver : public Archiver
{
public:
    explicit JsonArchiver(std::ostream& output);
    void archive() override;

protected:
    std::ostream& output;
    nlohmann::json archive_object;
    void add_property(const std::string& name, const serialization::Property& property) override;
};

class JsonDearchiver : public Dearchiver
{
public:
    explicit JsonDearchiver(std::istream& input);
    void dearchive() override;

protected:
    std::istream& input;
    nlohmann::json archive_object;
    bool get_property(const std::string& name, serialization::Property& out) override;
};
} // portal
