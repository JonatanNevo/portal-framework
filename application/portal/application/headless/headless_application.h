//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include "portal/application/application.h"
#include "portal/core/timer.h"

namespace portal {
    class HeadlessApplication final : public Application
    {
    public:
        HeadlessApplication(const ApplicationSpecs& specs = ApplicationSpecs());
        ~HeadlessApplication();

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
} // portal
