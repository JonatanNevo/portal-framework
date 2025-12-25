
.. _program_listing_file_portal_application_modules_module_stack.h:

Program Listing for File module_stack.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_application_modules_module_stack.h>` (``portal\application\modules\module_stack.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <memory>
   #include <span>
   #include <unordered_map>
   #include <unordered_set>
   #include <vector>
   
   #include "portal/application/frame_context.h"
   
   namespace portal
   {
   class Event;
   
   class BaseModule;
   
   // TODO: Move to application module
   // TODO: find a better name?
   // TODO: differentiate between using modules for dependency graph and memory allocation to using modules for polymorphisem flow control
   class ModuleStack
   {
   public:
       ~ModuleStack();
   
       void clean();
   
       template <typename T, typename... Args> requires std::is_base_of_v<BaseModule, T>
       T& add_module(Args&&... args)
       {
           auto& module = modules.emplace_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
           dependency_graph_dirty = true;
           return dynamic_cast<T&>(*module);
       }
   
       std::span<std::unique_ptr<BaseModule>> list_modules()
       {
           return modules;
       }
   
       void build_dependency_graph();
   
       void begin_frame(FrameContext& frame) const;
   
       void end_frame(FrameContext& frame) const;
   
       void update(FrameContext& frame) const;
   
       void gui_update(FrameContext& frame) const;
   
       void post_update(FrameContext& frame) const;
   
       void on_event(Event& event) const;
   
       [[nodiscard]] const std::vector<std::vector<BaseModule*>>& get_dependency_graph() const { return dependency_graph; }
   
   private:
       static int compute_module_level(
           BaseModule* module,
           std::unordered_map<BaseModule*, int>& levels,
           std::unordered_set<BaseModule*>& visited
       );
   
   private:
       std::vector<std::unique_ptr<BaseModule>> modules;
       std::vector<std::vector<BaseModule*>> dependency_graph;
       bool dependency_graph_dirty = false;
   
       // Pre-sorted vectors for efficient tag-based iteration
       // TODO: Split into multiple subclasses instead of having everything inside base_module
       std::vector<BaseModule*> frame_lifecycle_modules;
       std::vector<BaseModule*> update_modules;
       std::vector<BaseModule*> gui_update_modules;
       std::vector<BaseModule*> post_update_modules;
       std::vector<BaseModule*> event_modules;
   };
   } // portal
