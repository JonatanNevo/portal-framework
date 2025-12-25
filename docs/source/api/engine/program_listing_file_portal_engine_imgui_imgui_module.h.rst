
.. _program_listing_file_portal_engine_imgui_imgui_module.h:

Program Listing for File imgui_module.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_imgui_imgui_module.h>` (``portal\engine\imgui\imgui_module.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/application/modules/module.h"
   #include "portal/engine/renderer/renderer.h"
   
   namespace portal
   {
   class ImGuiModule : public TaggedModule<Tag<ModuleTags::FrameLifecycle, ModuleTags::GuiUpdate>, Renderer>
   {
   public:
       ImGuiModule(ModuleStack& stack, const Window& window);
       ~ImGuiModule() override;
   
       void begin_frame(FrameContext& frame) override;
       void end_frame(FrameContext& frame) override;
   
       void gui_update(FrameContext& frame) override;
   
   private:
       vk::raii::DescriptorPool imgui_pool = nullptr;
   };
   } // portal
