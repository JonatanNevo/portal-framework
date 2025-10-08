//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <nlohmann/json.hpp>

#include "portal/core/log.h"
#include "portal/engine/application/application.h"
#include "portal/engine/application/window.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/folder_resource_database.h"

using namespace portal;

int main()
{
    ApplicationSpecification spec;
    Application app{spec};
    app.run();
}
