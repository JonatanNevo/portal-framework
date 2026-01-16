//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <imgui.h>

#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/resources/resources/font.h"

namespace portal
{

struct ImGuiFontConfiguration
{
    StringId name;
    float size;
    ResourceReference<Font> font;
};

class ImGuiFonts
{
public:
    static void add(ImGuiFontConfiguration font, bool is_default = false, bool merge_with_last = false);
    static void push_font(const StringId& font_name);
    static void pop_font();
    static ImFont* get(const StringId& font_name);

private:
    inline static std::unordered_map<StringId, ImFont*> fonts{};
};

} // portal