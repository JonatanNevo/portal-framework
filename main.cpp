#include <portal/application/module/module_base.h>
#include <portal/application/application.h>
#include <portal/application/entry_point.h>

class TestModule: public portal::ModuleBase<portal::tags::Rendering>
{
public:
    TestModule(): ModuleBase("TestModule", {Hook::OnUpdate, Hook::OnAppStart, Hook::OnAppClose})
    {}

    void on_start(const portal::Configuration& config, portal::debug::DebugInfo& debug_info) override
    {
        this->debug_info = &debug_info;
        LOG_WARN("TestModule::on_start");
    }

    void on_update(float delta_time) override
    {
        for (const auto& field: debug_info->get_fields())
        {
            LOG_INFO("{}: {}", field->label, field->to_string());
        }
    }

    void on_close() override
    {
        LOG_WARN("TestModule::on_close");
    }

protected:
    portal::debug::DebugInfo* debug_info;
};

std::unique_ptr<portal::Application> create_application()
{
    const auto module = std::make_shared<TestModule>();
    auto application = std::make_unique<portal::Application>();
    application->add_module(module);
    return std::move(application);
}