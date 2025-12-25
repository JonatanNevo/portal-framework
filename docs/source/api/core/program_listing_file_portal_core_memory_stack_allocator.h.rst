
.. _program_listing_file_portal_core_memory_stack_allocator.h:

Program Listing for File stack_allocator.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_memory_stack_allocator.h>` (``portal\core\memory\stack_allocator.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <cstdint>
   #include <stdexcept>
   #include <unordered_map>
   #include <vector>
   #include <array>
   
   namespace portal
   {
   class StackAllocator
   {
   public:
       using marker = size_t;
   
       StackAllocator();
   
       explicit StackAllocator(size_t total_size);
   
       void* alloc(size_t size);
   
       template <typename T, typename... Args>
       T* alloc(Args&&... args)
       {
           void* mem = alloc(sizeof(T));
           return new(mem) T(std::forward<Args>(args)...);
       }
   
       template <typename T>
       void free(T* p)
       {
           if (p == nullptr)
               return;
   
           // Call the destructor
           p->~T();
   
           // Free the memory
           this->free(reinterpret_cast<void*>(p));
       }
   
       void free(void* p);
   
       [[nodiscard]] marker get_marker() const;
   
       [[nodiscard]] size_t get_size() const;
   
       void free_to_marker(marker m);
   
       void clear();
   
       void resize(size_t new_size);
   
   private:
       std::vector<uint8_t> buffer;
       marker top;
       std::unordered_map<void*, size_t> allocations;
   };
   
   
   template <unsigned int N> requires (N >= 2)
   class BufferedAllocator
   {
   public:
       BufferedAllocator() = default;
   
       explicit BufferedAllocator(const size_t buffer_size)
       {
           for (auto& allocator : allocators)
           {
               allocator.resize(buffer_size);
           }
       }
   
       void swap_buffers()
       {
           stack_index = (stack_index + 1) % N;
           allocators[stack_index].clear();
       }
   
       void* alloc(size_t size)
       {
           return allocators[stack_index].alloc(size);
       }
   
       template <typename T, typename... Args>
       T* alloc(Args&&... args)
       {
           void* mem = alloc(sizeof(T));
           return new(mem) T(std::forward<Args>(args)...);
       }
   
       void free(void* p)
       {
           allocators[stack_index].free(p);
       }
   
       template <typename T>
       void free(T* p)
       {
           if (p == nullptr)
               return;
   
           // Call the destructor
           p->~T();
   
           this->free(reinterpret_cast<void*>(p));
       }
   
       void clear()
       {
           allocators[stack_index].clear();
       }
   
       void clear(size_t)
       {
           allocators[stack_index].clear();
       }
   
       [[nodiscard]] StackAllocator& get_current_allocator()
       {
           return allocators[stack_index];
       }
   
       [[nodiscard]] StackAllocator& get_allocator(size_t index)
       {
           if (index >= N)
           {
               throw std::out_of_range("Index out of range for BufferedAllocator");
           }
           return allocators[index];
       }
   
   private:
       size_t stack_index = 0;
       std::array<StackAllocator, N> allocators;
   };
   
   using DoubleBufferedAllocator = BufferedAllocator<2>;
   } // portal
