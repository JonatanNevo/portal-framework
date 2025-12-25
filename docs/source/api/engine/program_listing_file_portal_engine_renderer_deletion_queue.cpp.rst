
.. _program_listing_file_portal_engine_renderer_deletion_queue.cpp:

Program Listing for File deletion_queue.cpp
===========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_deletion_queue.cpp>` (``portal\engine\renderer\deletion_queue.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "deletion_queue.h"
   
   namespace portal
   {
   void DeletionQueue::push_deleter(std::function<void()>&& deleter)
   {
       deletion_queue.push_back(deleter);
   }
   
   void DeletionQueue::flush()
   {
       // reverse iterate the deletion queue to execute all the functions
       for (auto it = deletion_queue.rbegin(); it != deletion_queue.rend(); ++it)
       {
           (*it)();
       }
   
       deletion_queue.clear();
   }
   } // portal
