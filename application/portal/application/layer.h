//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

namespace portal
{
class Application;
struct ApplicationContext;

class Layer
{
public:
    virtual ~Layer() = default;

    /**
     * Called whenever the layer is being attached to the application.
     * This is a virtual function and can be overridden to extend the functionally
     * Please call the base `on_attach` when overriding this function.
     *
     * @param app The `Application` this layer is attached to
     */
    virtual void on_attach(Application* app);

    /**
     * Called when this layer is removed from the application (usually when the application closes)
     * This is a virtual function and can be overridden to extend the functionally.
     */
    virtual void on_detach();

    /**
     * Called whenever the application context changed (usually when the window changes)
     * This is a virtual function and can be overridden to extend the functionally
     * Please call the base `on_context_change` when overriding this function.
     *
     * @param new_context The new `ApplicationContext` after the changes
     */
    virtual void on_context_change(ApplicationContext* new_context);

    /**
     * Application loop functions:
     * The application loop is built from 3 stages, pre_update, update, and post_update.
     * The `pre_update` determines if the layer should continue to the update,
     * this function should be as quick as possible.
     *
     * The `update` holds the main functionality of the layer for each frame, its `dt` is the same as `pre_update`'s `dt`
     *
     * Lastly `post_update` is called after *all layers* have finished their `update`s
     * Note that post_update `dt` can be different from `update` and `pre_update` due to this behavior
     */
    virtual bool pre_update(float dt);
    virtual void update(float dt);
    virtual void post_update(float dt);

    [[nodiscard]] bool valid() const { return application != nullptr && context != nullptr; }

protected:
    // TODO: don't use raw pointers? maybe use weak or shared instead?
    Application* application = nullptr;
    ApplicationContext* context = nullptr;
};
} // namespace portal
