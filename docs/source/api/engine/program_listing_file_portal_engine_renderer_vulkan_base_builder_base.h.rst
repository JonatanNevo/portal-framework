
.. _program_listing_file_portal_engine_renderer_vulkan_base_builder_base.h:

Program Listing for File builder_base.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_base_builder_base.h>` (``portal\engine\renderer\vulkan\base\builder_base.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <vulkan/vulkan.hpp>
   #include <vk_mem_alloc.h>
   
   namespace portal::renderer::vulkan
   {
   
   template <typename BuilderType, typename CreateInfoType>
   struct BuilderBase
   {
   public:
       virtual ~BuilderBase() = default;
   
       [[nodiscard]] const VmaAllocationCreateInfo& get_allocation_create_info() const
       {
           return alloc_create_info;
       }
   
       const CreateInfoType& get_create_info() const
       {
           return create_info;
       }
   
       [[nodiscard]] const std::string& get_debug_name() const
       {
           return debug_name;
       }
   
       BuilderType& with_debug_name(const std::string& name)
       {
           debug_name = name;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_implicit_sharing_mode()
       {
           create_info.sharingMode = (1 < create_info.queueFamilyIndexCount) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_memory_type_bits(uint32_t type_bits)
       {
           alloc_create_info.memoryTypeBits = type_bits;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_queue_families(uint32_t count, const uint32_t* family_indices)
       {
           create_info.queueFamilyIndexCount = count;
           create_info.pQueueFamilyIndices = family_indices;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_queue_families(const std::vector<uint32_t>& queue_families)
       {
           return with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data());
       }
   
       BuilderType& with_sharing_mode(vk::SharingMode sharing_mode)
       {
           create_info.sharingMode = sharing_mode;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_vma_flags(const VmaAllocationCreateFlags flags)
       {
           alloc_create_info.flags = flags;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_vma_pool(const VmaPool pool)
       {
           alloc_create_info.pool = pool;
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_vma_preferred_flags(const vk::MemoryPropertyFlags flags)
       {
           alloc_create_info.preferredFlags = static_cast<VkMemoryPropertyFlags>(flags);
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_vma_required_flags(const vk::MemoryPropertyFlags flags)
       {
           alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(flags);
           return static_cast<BuilderType&>(*this);
       }
   
       BuilderType& with_vma_usage(VmaMemoryUsage usage)
       {
           alloc_create_info.usage = usage;
           return static_cast<BuilderType&>(*this);
       }
   
       CreateInfoType& get_create_info()
       {
           return create_info;
       }
   
   protected:
       explicit BuilderBase(const CreateInfoType& create_info) : create_info(create_info)
       {
           alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
       }
   
   protected:
       VmaAllocationCreateInfo alloc_create_info = {};
       CreateInfoType create_info = {};
       std::string debug_name = {};
   };
   }
