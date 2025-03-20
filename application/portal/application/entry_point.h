//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include "platform/platform.h"

extern std::unique_ptr<portal::Platform> create_platform();
extern std::unique_ptr<portal::Application> create_application();

namespace portal
{
int platform_main(int argc, char** argv)
{
    const auto platform = create_platform();
    auto code = platform->initialize(create_application);

    if (code == ExitCode::Success)
    {
        code = platform->main_loop();
    }

    platform->terminate(code);
    return 0;
}
}


#if defined(PORTAL_PLATFORM_WINDOWS) && defined(PORTAL_GUI) && defined(PORTAL_DIST)

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    return portal::main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
    return portal::platform_main(argc, argv);
}

#endif
