//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/log.h"
#include "portal/engine/renderer/renderer.h"

int main()
{
    portal::Log::init({.default_log_level = portal::Log::LogLevel::Trace});
    try
    {
        portal::Renderer renderer;
        renderer.init();
        renderer.run();
        renderer.cleanup();
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Exception caught: {}", e.what());
    }
    catch (...)
    {
        LOG_FATAL("Fatal unknown exception caught");
    }
    portal::Log::shutdown();
}