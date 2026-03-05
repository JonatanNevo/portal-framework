//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/editor/editor_context.h"

namespace portal
{
/**
 * A set of functions for showing the component in the editor.
 *
 * @tparam T
 */
template <typename T>
struct ComponentEditorFunctions
{
    // If this component is removable or not
    static constexpr bool removable = true;
};

template <typename T>
concept HasOptionsFunction = requires(EditorContext& context, Entity entity) {
    { ComponentEditorFunctions<T>::draw_options(context, entity) };
};

template <typename T>
concept HasDetailsFunction = requires(EditorContext& context, Entity scene_entity, T& comp, std::span<const StringId> entities, bool is_multi_edit) {
    { ComponentEditorFunctions<T>::draw_details(context, scene_entity, comp, entities, is_multi_edit) };
};

template <typename T>
concept HasAddedFunction = requires(EditorContext& context, Entity entity) {
    { ComponentEditorFunctions<T>::on_added(context, entity) };
};

template <typename T>
concept HasRemovedFunction = requires(EditorContext& context, Entity entity) {
    { ComponentEditorFunctions<T>::on_removed(context, entity) };
};

template <typename T>
concept HasResetFunction = requires(EditorContext& context, Entity entity) {
    { ComponentEditorFunctions<T>::reset(context, entity) };
};
}
