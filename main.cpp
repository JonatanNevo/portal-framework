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

struct TestClass
{
    int a;
    float b;
    std::vector<int> c;
    std::string d;
    glm::vec1 e;
    glm::ivec2 f;
    glm::dvec3 g;
    glm::vec4 h;

    void serialize(portal::OrderedSerializer& serializer)
    {
        serializer << a << b << c << d << e << f << g << h;
    }

    static TestClass deserialize(portal::OrderedDeserializer& deserializer)
    {
        TestClass test;
        test.a = deserializer.get_property<int>();
        test.b = deserializer.get_property<float>();
        test.c = deserializer.get_property<std::vector<int>>();
        test.d = deserializer.get_property<std::string>();
        test.e = deserializer.get_property<glm::vec1>();
        test.f = deserializer.get_property<glm::ivec2>();
        test.g = deserializer.get_property<glm::dvec3>();
        test.h = deserializer.get_property<glm::vec4>();
        return test;
    }
};


portal::Application* portal::create_application(int argc, char** argv)
{
    Log::init();

    TestClass test = {
        5,
        3.14f,
        {1, 2, 3},
        "hello",
        glm::vec1{1.0f},
        glm::ivec2{1, 2},
        glm::dvec3{1.0, 2.0, 3.0},
        glm::vec4{1.0f, 2.0f, 3.0f, 4.0f}
    };
    std::stringstream ss;
    BinarySerializationParams params { true, true };
    BinarySerializer serializer(ss, params);
    serializer << test;

    serializer.serialize();
    std::string s = ss.str();
    std::cout << "size: " << s.size() << std::endl;

    BinaryDeserializer deserializer(s.data(), s.size());
    deserializer.deserialize();
    auto test2 = TestClass::deserialize(deserializer);
    std::cout << "a: " << test2.a << std::endl;
    std::cout << "b: " << test2.b << std::endl;
    std::cout << "c: " << test2.c << std::endl;
    std::cout << "d: " << test2.d << std::endl;
    std::cout << "e: " << test2.e << std::endl;
    std::cout << "f: " << test2.f << std::endl;
    std::cout << "g: " << test2.g << std::endl;
    std::cout << "h: " << test2.h << std::endl;

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
