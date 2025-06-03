//
// Created by thejo on 5/23/2025.
//

#include "layer.h"

#include "portal/application/application.h"

namespace portal
{
void Layer::on_attach(Application* app)
{
    application = app;
}

void Layer::on_detach() {}

void Layer::on_context_change(ApplicationContext* new_context)
{
    context = new_context;
}

bool Layer::pre_update(float) { return true; }

void Layer::update(float) {}

void Layer::post_update(float) {}
}
