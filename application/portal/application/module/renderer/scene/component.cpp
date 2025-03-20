//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "component.h"

namespace portal::scene_graph
{
Component::Component(const std::string& name): name(name)
{}

const std::string& Component::get_name() const
{
    return name;
}
} // portal