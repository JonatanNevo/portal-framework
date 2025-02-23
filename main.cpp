//
// Created by Jonatan Nevo on 31/01/2025.
//

#include <iostream>
#include <filesystem>
#include <fstream>

#include "serialization/portal/serialization/impl/binary_searilization.h"

#include <portal/gui/gui_application.h>
#include <portal/application/entry_point.h>



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

inline std::ostream& operator<<(std::ostream& os, const std::vector<int> vec)
{
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        os << vec[i];
        if (i != vec.size() - 1)
            os << ", ";
    }
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec1 vec)
{
    os << "(" << vec.x << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::ivec2 vec)
{
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::dvec3 vec)
{
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec4 vec)
{
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
    return os;
}


portal::Application* portal::create_application(int argc, char** argv)
{
    Log::init();

    std::stringstream ss;
    portal::BinarySerializationParams params { true, true };
    portal::BinarySerializer serializer(ss, params);
    int a = 5;
    serializer.add_property(a);
    float b = 3.14f;
    serializer.add_property(b);
    std::vector<int> c = {1, 2, 3};
    serializer.add_property(c);
    std::string d = "hello";
    serializer.add_property(d);
    glm::vec1 e{1.0f};
    serializer.add_property(e);
    glm::ivec2 f{1, 2};
    serializer.add_property(f);
    glm::dvec3 g{1.0, 2.0, 3.0};
    serializer.add_property(g);
    glm::vec4 h{1.0f, 2.0f, 3.0f, 4.0f};
    serializer.add_property(h);

    serializer.serialize();
    std::string s = ss.str();
    std::cout << "size: " << s.size() << std::endl;

    portal::BinaryDeserializer deserializer(s.data(), s.size());
    deserializer.deserialize();
    std::cout << "a: " << deserializer.get_property<int>() << std::endl;
    std::cout << "b: " << deserializer.get_property<float>() << std::endl;
    std::cout << "c: " << deserializer.get_property<std::vector<int>>() << std::endl;;
    std::cout << "d: " << deserializer.get_property<std::string>() << std::endl;
    std::cout << "e: " << deserializer.get_property<glm::vec1>() << std::endl;
    std::cout << "f: " << deserializer.get_property<glm::ivec2>() << std::endl;
    std::cout << "g: " << deserializer.get_property<glm::dvec3>() << std::endl;
    std::cout << "h: " << deserializer.get_property<glm::vec4>() << std::endl;

    exit(0);

    portal::ApplicationSpecs specs{
        .name = "Example App"
    };

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
