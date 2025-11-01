//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesh.h"

namespace portal
{

namespace resources
{
    MeshData& MeshData::operator=(std::nullptr_t)
    {
        index_buffer = nullptr;
        vertex_buffer = nullptr;
        return *this;
    }

}
} // portal

