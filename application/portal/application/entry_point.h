//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

extern portal::Application* portal::create_application(int arc, char** argv);
bool g_application_running = true;

namespace portal
{
int main(int argc, char** argv)
{
    while (g_application_running)
    {
        Application* app = create_application(argc, argv);
        app->run();
        delete app;
    }

    return 0;
}
}

#if defined(PORTAL_PLATFORM_WINDOWS) && !defined(PORTAL_HEADLESS) && defined(PORTAL_DIST)

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    return portal::main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
    return portal::main(argc, argv);
}

#endif
