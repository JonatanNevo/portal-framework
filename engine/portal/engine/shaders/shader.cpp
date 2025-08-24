//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader.h"

namespace portal
{
void Shader::copy_from(const Ref<Resource> other)
{
    auto other_shader = other.as<Shader>();
    reflection = other_shader->reflection;
    code = other_shader->code;
}

const std::string& Shader::get_entry_point(const ShaderStage stage) const
{
    return reflection.entry_points.at(stage);
}
} // portal
