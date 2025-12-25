
.. _program_listing_file_portal_engine_editor_editor_module.h:

Program Listing for File editor_module.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_editor_editor_module.h>` (``portal\engine\editor\editor_module.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "editor_system.h"
   #include "portal/application/modules/module.h"
   #include "portal/engine/renderer/renderer.h"
   
   namespace portal
   {
   class EditorModule final : public TaggedModule<Tag<ModuleTags::GuiUpdate>, Renderer, SystemOrchestrator>
   {
   public:
       explicit EditorModule(ModuleStack& stack);
   
       void gui_update(FrameContext& frame) override;
   
   private:
       EditorGuiSystem gui_system;
   };
   } // portal
