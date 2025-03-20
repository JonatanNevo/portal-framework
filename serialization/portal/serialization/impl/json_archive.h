//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include <nlohmann/json.hpp>
#include "portal/serialization/archive.h"

namespace portal
{
class JsonArchiver final : public Archiver
{
public:
    explicit JsonArchiver(std::ostream& output);
    void archive() override;

protected:
    std::ostream& output;
    nlohmann::json archive_object;
    void add_property(const std::string& name, const serialization::Property& property) override;
};

class JsonDearchiver final : public Dearchiver
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
