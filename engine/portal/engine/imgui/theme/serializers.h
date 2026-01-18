//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "imgui_internal.h"

#include <portal/serialization/archive.h>
#include <portal/serialization/serialize.h>

namespace portal
{
template <>
struct Archivable<ImVec4>
{
    static void archive(const ImVec4& vec, ArchiveObject& archive);
    static ImVec4 dearchive(ArchiveObject& archive);
};

template <>
struct Archivable<ImVec2>
{
    static void archive(const ImVec2& vec, ArchiveObject& archive);
    static ImVec2 dearchive(ArchiveObject& archive);
};

template <>
struct Archivable<ImGuiStyle>
{
    static void archive(const ImGuiStyle& style, ArchiveObject& archive);
    static ImGuiStyle dearchive(ArchiveObject& archive);
};
} // portal
