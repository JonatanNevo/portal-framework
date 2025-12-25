
.. _program_listing_file_portal_engine_resources_reference_manager.h:

Program Listing for File reference_manager.h
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_reference_manager.h>` (``portal\engine\resources\reference_manager.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <unordered_set>
   
   #include "llvm/ADT/DenseMap.h"
   #include "llvm/ADT/SmallSet.h"
   #include "portal/application/modules/module.h"
   #include "portal/core/concurrency/reentrant_spin_lock.h"
   #include "portal/core/concurrency/spin_lock.h"
   
   #include "resources/resource.h"
   
   namespace portal
   {
   class ReferenceManager final : public Module<>
   {
   public:
       explicit ReferenceManager(ModuleStack& stack);
   
       ~ReferenceManager();
   
       void register_reference(const StringId& id, void* reference);
   
       void unregister_reference(const StringId& id, void* reference);
   
       void move_reference(const StringId& id, void* old_ref, void* new_ref);
   
   private:
       ReentrantSpinLock<> lock;
   #ifdef PORTAL_DEBUG
       std::unordered_map<StringId, std::unordered_set<void*>> references;
   #else
       llvm::DenseMap<StringId, llvm::SmallSet<void*, 16>> references;
   #endif
   };
   } // portal
