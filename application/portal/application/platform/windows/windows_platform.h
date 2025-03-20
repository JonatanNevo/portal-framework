//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include "portal/application/platform/platform.h"

namespace portal
{

class WindowsPlatform final : public Platform
{
protected:
    void create_window(const Window::Properties &properties) override;
};

}

std::unique_ptr<portal::Platform> create_platform();


