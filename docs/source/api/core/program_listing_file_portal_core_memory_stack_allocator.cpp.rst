
.. _program_listing_file_portal_core_memory_stack_allocator.cpp:

Program Listing for File stack_allocator.cpp
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_memory_stack_allocator.cpp>` (``portal\core\memory\stack_allocator.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "portal/core/memory/stack_allocator.h"
   
   #include <stdexcept>
   
   namespace portal
   {
   constexpr size_t DEFAULT_SIZE = 1024;
   
   StackAllocator::StackAllocator() : StackAllocator(DEFAULT_SIZE)
   {}
   
   StackAllocator::StackAllocator(const size_t total_size) :
       buffer(total_size),
       top(0) {}
   
   void* StackAllocator::alloc(const size_t size)
   {
       if (top + size > buffer.size())
       {
           throw std::bad_alloc(); // Not enough space
       }
   
       void* p = buffer.data() + top;
       allocations[p] = size;
       top += size;
   
       return p;
   }
   
   void StackAllocator::free(void* p)
   {
       if (!allocations.contains(p))
       {
           throw std::invalid_argument("Pointer not allocated by this stack allocator");
       }
   
       const size_t size = allocations[p];
       allocations.erase(p);
       top -= size;
   }
   
   StackAllocator::marker StackAllocator::get_marker() const
   {
       return top;
   }
   
   size_t StackAllocator::get_size() const
   {
       return buffer.size();
   }
   
   void StackAllocator::free_to_marker(const marker m)
   {
       // Ignores the allocations map, worst case we will override it.
       top = m;
   }
   
   void StackAllocator::clear()
   {
       top = 0;
       allocations.clear();
   }
   
   void StackAllocator::resize(const size_t new_size)
   {
       clear();
       buffer.resize(new_size);
   }
   } // portal
