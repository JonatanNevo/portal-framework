//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"
#include "portal/engine/renderer/pipeline/pipeline_types.h"

namespace portal::renderer
{

class Pipeline : public RefCounted
{
public:
    [[nodiscard]] virtual pipeline::Specification& get_spec() = 0;
    [[nodiscard]] virtual const pipeline::Specification& get_spec() const = 0;

    virtual void initialize() = 0;

    [[nodiscard]] virtual Ref<Shader> get_shader() const = 0;
};
} // portal