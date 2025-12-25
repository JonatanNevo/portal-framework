
.. _program_listing_file_portal_engine_renderer_deletion_queue.h:

Program Listing for File deletion_queue.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_deletion_queue.h>` (``portal\engine\renderer\deletion_queue.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <deque>
   #include <functional>
   
   
   namespace portal
   {
   class DeletionQueue
   {
   public:
       void push_deleter(std::function<void()>&& deleter);
       void flush();
   
   private:
       std::deque<std::function<void()>> deletion_queue;
   };
   } // portal
