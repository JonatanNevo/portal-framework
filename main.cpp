//
// Created by Jonatan Nevo on 31/01/2025.
//


#include <filesystem>
#include <fstream>

#include "portal/application/application.h"
#include "portal/networking/connection.h"
#include "portal/renderer/renderer.h"
#include "portal/renderer/ui/ui_renderable.h"
#include "portal/serialization/impl/binary_searilization.h"
#include "portal/application/entry_point.h"


class TestLayer : public portal::UIRenderable
{
public:
    virtual void on_ui_render() override
    {
        ImGui::Begin("Hello");
        ImGui::Button("Button");
        ImGui::End();

        ImGui::ShowDemoWindow();

        UI_DrawAboutModal();
    }

    void UI_DrawAboutModal()
    {
        if (!m_AboutModalOpen)
            return;

        ImGui::OpenPopup("About");
        m_AboutModalOpen = ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (m_AboutModalOpen)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);

            ImGui::BeginGroup();
            ImGui::Text("Portal application framework");
            ImGui::EndGroup();

            ImGuiStyle& style = ImGui::GetStyle();

            float actualSize = ImGui::CalcTextSize("Close").x + style.FramePadding.x * 2.0f;
            float avail = ImGui::GetContentRegionAvail().x;

            float off = (avail - actualSize) * 0.5f;
            if (off > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

            if (ImGui::Button("Close"))
            {
                m_AboutModalOpen = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void ShowAboutModal()
    {
        m_AboutModalOpen = true;
    }

private:
    bool m_AboutModalOpen = false;
};

portal::Application* portal::create_application(int argc, char** argv)
{
    Log::init();
    ApplicationSettings specs{
        .name = "Example App"
    };
    auto renderer = std::make_shared<portal::Renderer>();
    auto* app = new Application(specs);
    app->push_layer(renderer);
    std::shared_ptr<TestLayer> layer = std::make_shared<TestLayer>();
    renderer->add_ui_renderable(layer);

    return app;
}
