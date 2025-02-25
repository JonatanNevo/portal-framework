//
// Created by Jonatan Nevo on 31/01/2025.
//


#include <filesystem>
#include <fstream>
#include <iostream>

#include <portal/gui/gui_application.h>
#include <portal/application/entry_point.h>

#include "portal/core/assert.h"
#include "portal/core/buffer.h"
#include "portal/core/buffer_stream.h"
#include "portal/serialization/impl/binary_searilization.h"


class TestLayer : public portal::Layer
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
    ApplicationSpecs specs{
        .name = "Example App"
    };

    std::vector<uint8_t> data(100);
    Buffer buffer(data.data(), data.size());
    BufferStreamWriter writer(buffer);
    std::map<int, std::string> mapping = {
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };
    BinarySerializer serializer(writer);
    serializer << mapping;

    BufferStreamReader reader(buffer);
    BinaryDeserializer deserializer(reader);
    std::map<int, std::string> deserialized;
    deserializer >> deserialized;

    auto* app = new GUIApplication(specs);
    std::shared_ptr<TestLayer> layer = std::make_shared<TestLayer>();
    app->push_layer(layer);
    app->set_menubar_callback(
        [app, layer]()
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                {
                    app->close();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                    layer->ShowAboutModal();
                }
                ImGui::EndMenu();
            }
        }
    );
    return app;
}
