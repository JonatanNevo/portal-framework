
.. _program_listing_file_portal_engine_editor_editor_module.cpp:

Program Listing for File editor_module.cpp
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_editor_editor_module.cpp>` (``portal\engine\editor\editor_module.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "editor_module.h"
   
   namespace portal
   {
   EditorModule::EditorModule(ModuleStack& stack) : TaggedModule(stack, STRING_ID("Editor Module")) {}
   
   void EditorModule::gui_update(FrameContext& frame)
   {
       gui_system.execute(*frame.ecs_registry);
   }
   } // portal
