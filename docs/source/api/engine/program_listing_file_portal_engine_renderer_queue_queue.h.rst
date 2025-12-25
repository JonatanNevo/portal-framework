
.. _program_listing_file_portal_engine_renderer_queue_queue.h:

Program Listing for File queue.h
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_queue_queue.h>` (``portal\engine\renderer\queue\queue.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   namespace portal::renderer
   {
   class Queue
   {
   public:
       virtual ~Queue() = default;
   
       [[nodiscard]] virtual size_t get_index() const = 0;
   
       // TODO: add `submit` `present`
   };
   }
