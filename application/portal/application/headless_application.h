//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/timer.h>

#include "portal/application/application.h"

namespace portal
{
class HeadlessApplication final : public Application
{
public:
    explicit HeadlessApplication(ApplicationSpecs specs = ApplicationSpecs());
    ~HeadlessApplication() override;

    static HeadlessApplication& get();

    void run() override;

    void close() override;

    float get_time() override;

private:
    void init();
    void shutdown();

private:
    ApplicationSpecs specs;
    bool running = false;

    float time_step = 0.f;
    float frame_time = 0.f;
    float last_frame_time = 0.f;
    Timer app_timer;
};
} // namespace portal
