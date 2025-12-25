
.. _program_listing_file_portal_engine_renderer_vulkan_base_allocated.h:

Program Listing for File allocated.h
====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_base_allocated.h>` (``portal\engine\renderer\vulkan\base\allocated.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <vk_mem_alloc.h>
   #include <vulkan/vulkan_raii.hpp>
   
   #include "portal/core/buffer.h"
   #include "portal/engine/renderer/vulkan/vulkan_common.h"
   #include "portal/engine/renderer/vulkan/base/allocated.h"
   #include "portal/engine/renderer/vulkan/base/vulkan_resource.h"
   
   namespace portal::renderer::vulkan::allocation
   {
   VmaAllocator& get_vma_allocator();
   
   void init(const vk::Instance& instance, const vk::PhysicalDevice& physical_device, const vk::Device& device);
   
   void shutdown();
   
   
   template <typename HandleType>
   class Allocated : public VulkanResource<HandleType>
   {
   public:
       Allocated() = delete;
       Allocated(const Allocated&) = delete;
       Allocated(Allocated&& other) noexcept;
       Allocated& operator=(Allocated const& other) = delete;
       Allocated& operator=(Allocated&& other) = default;
       virtual Allocated& operator=(std::nullptr_t);
   
   protected:
       template <typename... Args>
       explicit Allocated(const VmaAllocationCreateInfo& allocation_create_info, Args&&... args);
   
       explicit Allocated(HandleType handle, vk::raii::Device* device_ = nullptr);
   
   public:
       const HandleType* get() const;
   
       void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = vk::WholeSize) const;
   
       const void* get_data() const;
   
       void* get_data();
   
       vk::DeviceMemory get_memory() const;
   
       virtual uint8_t* map();
   
       bool mapped() const;
   
       void unmap();
   
       size_t update(const uint8_t* data, size_t size, size_t offset = 0) const;
   
       size_t update(const void* data, size_t size, size_t offset = 0) const;
   
       template <typename T>
       size_t update(const std::vector<T>& data, const size_t offset = 0)
       {
           return update(static_cast<const void*>(data.data()), data.size() * sizeof(T), offset);
       }
   
       template <typename T, size_t N>
       size_t update(const std::array<T, N>& data, const size_t offset = 0)
       {
           return update(data.data(), data.size() * sizeof(T), offset);
       }
   
       size_t update(const portal::Buffer& buffer, const size_t offset = 0) const;
   
       template <class T>
       size_t convert_and_update(const T& object, const size_t offset = 0)
       {
           return update(reinterpret_cast<const uint8_t*>(&object), sizeof(T), offset);
       }
   
       template <class T>
       size_t update_typed(const vk::ArrayProxy<T>& object, const size_t offset = 0)
       {
           return update(reinterpret_cast<const uint8_t*>(object.data()), object.size() * sizeof(T), offset);
       }
   
   protected:
       [[nodiscard]] vk::Buffer create_buffer(const vk::BufferCreateInfo& create_info);
   
       [[nodiscard]] vk::Image create_image(const vk::ImageCreateInfo& create_info);
   
       virtual void post_create(const VmaAllocationInfo& allocation_info);
   
       void destroy_buffer(vk::Buffer buffer);
   
       void destroy_image(vk::Image image);
   
       void clear();
   
   private:
       VmaAllocationCreateInfo allocation_create_info = {};
       VmaAllocation allocation = nullptr;
   
       uint8_t* mapped_data = nullptr;
   
       bool coherent = false;
   
       bool persistent = false;
   };
   
   template <typename HandleType>
   Allocated<HandleType>::Allocated(Allocated&& other) noexcept
       : VulkanResource<HandleType>(static_cast<VulkanResource<HandleType>&&>(other)),
         allocation_create_info(std::exchange(other.allocation_create_info, {})),
         allocation(std::exchange(other.allocation, {})),
         mapped_data(std::exchange(other.mapped_data, {})),
         coherent(std::exchange(other.coherent, {})),
         persistent(std::exchange(other.persistent, {}))
   {}
   
   template <typename HandleType>
   Allocated<HandleType>& Allocated<HandleType>::operator=(std::nullptr_t)
   {
       allocation_create_info = {};
       allocation = nullptr;
       mapped_data = nullptr;
       coherent = false;
       persistent = false;
       return *static_cast<Allocated*>(this);
   }
   
   template <typename HandleType>
   template <typename... Args>
   Allocated<HandleType>::Allocated(const VmaAllocationCreateInfo& allocation_create_info, Args&&... args) :
       VulkanResource<HandleType>(std::forward<Args>(args)...),
       allocation_create_info(allocation_create_info) {}
   
   template <typename HandleType>
   Allocated<HandleType>::Allocated(HandleType handle, vk::raii::Device* device_) : VulkanResource<HandleType>(handle, device_) {}
   
   template <typename HandleType>
   const HandleType* Allocated<HandleType>::get() const
   {
       return &VulkanResource<HandleType>::get_handle();
   }
   
   template <typename HandleType>
   void Allocated<HandleType>::flush(const vk::DeviceSize offset, const vk::DeviceSize size) const
   {
       if (!coherent)
           vmaFlushAllocation(get_vma_allocator(), allocation, offset, size);
   }
   
   template <typename HandleType>
   const void* Allocated<HandleType>::get_data() const
   {
       return mapped_data;
   }
   
   template <typename HandleType>
   void* Allocated<HandleType>::get_data()
   {
       return mapped_data;
   }
   
   template <typename HandleType>
   vk::DeviceMemory Allocated<HandleType>::get_memory() const
   {
       VmaAllocationInfo allocation_info = {};
       vmaGetAllocationInfo(get_vma_allocator(), allocation, &allocation_info);
       return allocation_info.deviceMemory;
   }
   
   template <typename HandleType>
   uint8_t* Allocated<HandleType>::map()
   {
       if (!persistent && !mapped())
       {
           const auto result = vmaMapMemory(get_vma_allocator(), allocation, reinterpret_cast<void**>(&mapped_data));
           PORTAL_ASSERT(result == VK_SUCCESS, "Failed to map memory");
       }
       else
           LOG_WARN_TAG("Vulkan", "Attempting to map a persistent or mapped memory");
       return mapped_data;
   }
   
   template <typename HandleType>
   bool Allocated<HandleType>::mapped() const
   {
       return mapped_data != nullptr;
   }
   
   template <typename HandleType>
   void Allocated<HandleType>::unmap()
   {
       if (!persistent && mapped())
       {
           vmaUnmapMemory(get_vma_allocator(), allocation);
           mapped_data = nullptr;
       }
   }
   
   template <typename HandleType>
   size_t Allocated<HandleType>::update(const uint8_t* data, const size_t size, const size_t offset) const
   {
       if (persistent)
       {
           std::copy_n(data, size, mapped_data + offset);
           flush();
       }
       else
       {
           LOG_WARN_TAG("Vulkan", "Attempting to update a persistent memory");
       }
       return size;
   }
   
   template <typename HandleType>
   size_t Allocated<HandleType>::update(const void* data, const size_t size, const size_t offset) const
   {
       return update(static_cast<const uint8_t*>(data), size, offset);
   }
   
   template <typename HandleType>
   size_t Allocated<HandleType>::update(const portal::Buffer& buffer, const size_t offset) const
   {
       return update(buffer.data, buffer.size, offset);
   }
   
   template <typename HandleType>
   void Allocated<HandleType>::clear()
   {
       mapped_data = nullptr;
       persistent = false;
       allocation_create_info = VmaAllocationCreateInfo{};
   }
   
   template <typename HandleType>
   vk::Buffer Allocated<HandleType>::create_buffer(const vk::BufferCreateInfo& create_info)
   {
       VmaAllocationInfo allocation_info{};
       VkBuffer buffer_handle = nullptr;
       VmaAllocation new_allocation = nullptr;
       const auto result = vmaCreateBuffer(
           get_vma_allocator(),
           reinterpret_cast<const VkBufferCreateInfo*>(&create_info),
           &allocation_create_info,
           &buffer_handle,
           &new_allocation,
           &allocation_info
       );
   
       const auto buffer = vk::Buffer(buffer_handle);
       if (result != VK_SUCCESS)
       {
           throw std::runtime_error("failed to create buffer");
       }
       this->allocation = new_allocation;
       post_create(allocation_info);
       return buffer;
   }
   
   template <typename HandleType>
   vk::Image Allocated<HandleType>::create_image(const vk::ImageCreateInfo& create_info)
   {
       PORTAL_ASSERT(0 < create_info.mipLevels, "Image must have at least one mip level");
       PORTAL_ASSERT(0 < create_info.arrayLayers, "Image must have at least one array layer");
       PORTAL_ASSERT(create_info.usage, "Image must have at least one usage type");
   
       VmaAllocationInfo allocation_info{};
       VkImage image_handle;
       VmaAllocation new_allocation = nullptr;
       const auto result = vmaCreateImage(
           get_vma_allocator(),
           reinterpret_cast<const VkImageCreateInfo*>(&create_info),
           &allocation_create_info,
           &image_handle,
           &new_allocation,
           &allocation_info
       );
   
       const auto image = vk::Image(image_handle);
   
       if (result != VK_SUCCESS)
       {
           throw std::runtime_error("failed to create image");
       }
       this->allocation = new_allocation;
       post_create(allocation_info);
       return image;
   }
   
   template <typename HandleType>
   void Allocated<HandleType>::post_create(const VmaAllocationInfo& allocation_info)
   {
       VkMemoryPropertyFlags memory_properties_raw{};
       vmaGetAllocationMemoryProperties(get_vma_allocator(), allocation, &memory_properties_raw);
   
       const auto memory_properties = vk::MemoryPropertyFlags(memory_properties_raw);
       coherent = (memory_properties & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlagBits::eHostCoherent;
       mapped_data = static_cast<uint8_t*>(allocation_info.pMappedData);
       persistent = mapped();
   }
   
   template <typename HandleType>
   void Allocated<HandleType>::destroy_buffer(const vk::Buffer buffer)
   {
       if (buffer != nullptr && allocation)
       {
           unmap();
           vmaDestroyBuffer(get_vma_allocator(), buffer, this->allocation);
           clear();
       }
   }
   
   template <typename HandleType>
   void Allocated<HandleType>::destroy_image(const vk::Image image)
   {
       if (image != nullptr && allocation)
       {
           unmap();
           vmaDestroyImage(get_vma_allocator(), image, this->allocation);
           clear();
       }
   }
   } // portal
