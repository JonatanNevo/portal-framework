//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#define VK_HANDLE_CAST(raii_obj) reinterpret_cast<uint64_t>(static_cast<decltype(raii_obj)::CType>(*(raii_obj)))