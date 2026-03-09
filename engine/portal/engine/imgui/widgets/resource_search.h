//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <imgui.h>
#include <string_view>

#include "portal/engine/editor/editor_context.h"

namespace portal::imgui
{

bool resource_search_popup(
    EditorContext& context,
    std::string_view popup_id,
    ResourceType type,
    ResourceReference<Resource>& selected,
    bool* cleared = nullptr,
    std::string_view hint = "Search Resources",
    ImVec2 size = ImVec2{250.f, 350.f}
);

bool resource_search_popup(
    EditorContext& context,
    std::string_view popup_id,
    ResourceReference<Resource>& selected,
    bool* cleared = nullptr,
    std::string_view hint = "Search Resources",
    ImVec2 size = ImVec2{250.f, 350.f},
    std::initializer_list<ResourceType> types = {}
);

}
