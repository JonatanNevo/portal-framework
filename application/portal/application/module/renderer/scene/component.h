//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once

#include <string>
#include <typeindex>

namespace portal::scene_graph
{
/// @brief A generic class which can be used by nodes.
class Component
{
public:
    Component() = default;
    explicit Component(const std::string &name);
    Component(Component &&other) = default;
    virtual ~Component() = default;

    const std::string &get_name() const;
    virtual std::type_index get_type() = 0;
private:
    std::string name;
};
} // portal
