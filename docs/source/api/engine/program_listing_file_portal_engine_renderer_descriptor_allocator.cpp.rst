
.. _program_listing_file_portal_engine_renderer_descriptor_allocator.cpp:

Program Listing for File descriptor_allocator.cpp
=================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptor_allocator.cpp>` (``portal\engine\renderer\descriptor_allocator.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "descriptor_allocator.h"
   
   #include "portal/core/debug/assert.h"
   
   namespace portal::renderer::vulkan
   {
   DescriptorAllocator::DescriptorAllocator(const vk::raii::Device& device, const uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios)
       : sets_per_pool(static_cast<uint32_t>(max_sets * 1.5)),
         device(device)
   {
       ratios.append_range(pool_ratios);
   
       auto new_pool = create_pool(max_sets, pool_ratios);
       ready_pools.push_back(std::move(new_pool));
   }
   
   DescriptorAllocator::DescriptorAllocator(DescriptorAllocator&& other) noexcept
       : ratios(std::exchange(other.ratios, {})),
         full_pools(std::exchange(other.full_pools, {})),
         ready_pools(std::exchange(other.ready_pools, {})),
         sets_per_pool(std::exchange(other.sets_per_pool, 0)),
         device(other.device)
   {}
   
   DescriptorAllocator& DescriptorAllocator::operator=(DescriptorAllocator&& other) noexcept
   {
       if (&other == this)
           return *this;
   
       PORTAL_ASSERT(&device == &other.device, "Cannot move from one device to another");
   
       clear_pools();
       ratios = std::exchange(other.ratios, {});
       full_pools = std::exchange(other.full_pools, {});
       ready_pools = std::exchange(other.ready_pools, {});
       sets_per_pool = std::exchange(other.sets_per_pool, 0);
   
       return *this;
   }
   
   void DescriptorAllocator::clear_pools()
   {
       for (auto& p : ready_pools)
       {
           p.reset();
       }
   
       for (auto& p : full_pools)
       {
           p.reset();
           ready_pools.push_back(std::move(p));
       }
       full_pools.clear();
   }
   
   void DescriptorAllocator::destroy_pools()
   {
       ready_pools.clear();
       full_pools.clear();
   }
   
   std::vector<vk::raii::DescriptorSet> DescriptorAllocator::handle_pool_resize(
       vk::raii::DescriptorPool& descriptor_pool,
       vk::DescriptorSetAllocateInfo& info
   )
   {
       full_pools.push_back(std::move(descriptor_pool));
       descriptor_pool = get_pool();
       info.descriptorPool = descriptor_pool;
       return device.allocateDescriptorSets(info);
   }
   
   vk::raii::DescriptorSet DescriptorAllocator::allocate(const vk::DescriptorSetLayout& layout)
   {
       vk::raii::DescriptorPool descriptor_pool = get_pool();
       vk::DescriptorSetAllocateInfo info{
           .descriptorPool = descriptor_pool,
           .descriptorSetCount = 1,
           .pSetLayouts = &layout
       };
   
       std::vector<vk::raii::DescriptorSet> sets;
       try
       {
           sets = device.allocateDescriptorSets(info);
       }
       catch (vk::OutOfPoolMemoryError&)
       {
           sets = handle_pool_resize(descriptor_pool, info);
       }
       catch (vk::FragmentedPoolError&)
       {
           sets = handle_pool_resize(descriptor_pool, info);
       }
   
       ready_pools.push_back(std::move(descriptor_pool));
       return std::move(sets.front());
   }
   
   vk::raii::DescriptorPool DescriptorAllocator::get_pool()
   {
       vk::raii::DescriptorPool new_pool = nullptr;
       if (!ready_pools.empty())
       {
           new_pool = std::move(ready_pools.back());
           ready_pools.pop_back();
       }
       else
       {
           new_pool = create_pool(sets_per_pool, ratios);
   
           sets_per_pool = static_cast<uint32_t>(sets_per_pool * 1.5);
           if (sets_per_pool > 4092)
               sets_per_pool = 4092;
       }
       return new_pool;
   }
   
   vk::raii::DescriptorPool DescriptorAllocator::create_pool(const uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) const
   {
       std::vector<vk::DescriptorPoolSize> pool_sizes;
       pool_sizes.reserve(pool_ratios.size());
       for (auto& [type, ratio] : pool_ratios)
       {
           pool_sizes.push_back(
               {
                   .type = type,
                   .descriptorCount = static_cast<uint32_t>(set_count * ratio)
               }
           );
       }
   
       const vk::DescriptorPoolCreateInfo pool_info{
           .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
           .maxSets = set_count,
           .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
           .pPoolSizes = pool_sizes.data()
       };
       return device.createDescriptorPool(pool_info);
   }
   } // portal
