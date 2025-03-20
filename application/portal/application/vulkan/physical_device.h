//
// Created by Jonatan Nevo on 02/03/2025.
//

#pragma once

#include "base/vulkan_resource.h"
#include "instance.h"

namespace portal::vulkan
{
struct DriverVersion
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
};

class PhysicalDevice
{
public:
    PhysicalDevice(Instance& instance, vk::PhysicalDevice physical_device);

    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(PhysicalDevice&&) = delete;

    /**
     * @return The version of the driver
     */
    [[nodiscard]] DriverVersion get_driver_version() const;

    /**
     * @brief Used at logical device creation to pass the extensions feature chain to vkCreateDevice
     * @returns A void pointer to the start of the extension linked list
     */
    [[nodiscard]] void* get_extension_feature_chain() const;

    [[nodiscard]] bool is_extension_supported(const std::string& requested_extension) const;

    [[nodiscard]] const vk::PhysicalDeviceFeatures& get_features() const { return features; }
    [[nodiscard]] vk::PhysicalDevice get_handle() const { return handle; }
    [[nodiscard]] Instance& get_instance() const { return instance; }
    [[nodiscard]] const vk::PhysicalDeviceMemoryProperties& get_memory_properties() const { return memory_properties; }
    [[nodiscard]] const vk::PhysicalDeviceProperties& get_properties() const { return properties; }
    [[nodiscard]] const std::vector<vk::QueueFamilyProperties>& get_queue_family_properties() const { return queue_family_properties; }
    [[nodiscard]] vk::PhysicalDeviceFeatures get_requested_features() const { return requested_features; }
    vk::PhysicalDeviceFeatures& get_mutable_requested_features() { return requested_features; }

    uint32_t get_queue_family_performance_query_passes(
        const vk::QueryPoolPerformanceCreateInfoKHR* perf_query_create_info
    ) const;


    /**
     * @brief Checks that a given memory type is supported by the GPU
     * @param bits The memory requirement type bits
     * @param properties The memory property to search for
     * @param memory_type_found True if found, false if not found
     * @returns The memory type index of the found memory type
     */
    uint32_t get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32* memory_type_found = nullptr) const;

    /**
     * @brief Get an extension features struct
     *
     *        Gets the actual extension features struct with the supported flags set.
     *        The flags you're interested in can be set in a corresponding struct in the structure chain
     *        by calling PhysicalDevice::add_extension_features()
     * @returns The extension feature struct
     */
    template <typename StructureType>
    StructureType get_extension_features()
    {
        // We cannot request extension features if the physical device properties 2 instance extension isn't enabled
        if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        {
            throw std::runtime_error(
                "Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
                " isn't enabled!"
            );
        }

        // Get the extension feature
        return handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, StructureType>().template get<StructureType>();
    }

    /**
     * @brief Add an extension features struct to the structure chain used for device creation
     *
     *        To have the features enabled, this function must be called before the logical device
     *        is created. To do this request sample specific features inside
     *        VulkanSample::request_gpu_features(vkb::HPPPhysicalDevice &gpu).
     *
     *        If the feature extension requires you to ask for certain features to be enabled, you can
     *        modify the struct returned by this function, it will propagate the changes to the logical
     *        device.
     * @returns A reference to the extension feature struct in the structure chain
     */
    template <typename StructureType>
    StructureType& add_extension_features()
    {
        // We cannot request extension features if the physical device properties 2 instance extension isn't enabled
        if (!instance.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        {
            throw std::runtime_error(
                "Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
                " isn't enabled!"
            );
        }

        // Add an (empty) extension features into the map of extension features
        auto [it, added] = extension_features.insert({StructureType::structureType, std::make_shared<StructureType>()});
        if (added)
        {
            // if it was actually added, also add it to the structure chain
            if (last_requested_extension_feature)
                static_cast<StructureType*>(it->second.get())->pNext = last_requested_extension_feature;
            last_requested_extension_feature = it->second.get();
        }

        return *static_cast<StructureType*>(it->second.get());
    }

    /**
     * @brief Request an optional features flag
     *
     *        Calls get_extension_features to get the support of the requested flag. If it's supported,
     *        add_extension_features is called, otherwise a log message is generated.
     *
     * @returns true if the requested feature is supported, otherwise false
     */
    template <typename Feature>
    vk::Bool32 request_optional_feature(vk::Bool32 Feature::* flag, std::string const& featureName, std::string const& flagName)
    {
        const vk::Bool32 supported = get_extension_features<Feature>().*flag;
        if (supported)
            add_extension_features<Feature>().*flag = true;
        else
            LOG_CORE_INFO_TAG("Vulkan", "Requested optional feature <{}::{}> is not supported", featureName, flagName);
        return supported;
    }

    /**
     * @brief Request a required features flag
     *
     *        Calls get_extension_features to get the support of the requested flag. If it's supported,
     *        add_extension_features is called, otherwise a runtime_error is thrown.
     */
    template <typename Feature>
    void request_required_feature(vk::Bool32 Feature::* flag, std::string const& featureName, std::string const& flagName)
    {
        if (get_extension_features<Feature>().*flag)
            add_extension_features<Feature>().*flag = true;
        else
            throw std::runtime_error(std::string("Requested required feature <") + featureName + "::" + flagName + "> is not supported");
    }

    /**
     * @brief Sets whether or not the first graphics queue should have higher priority than other queues.
     * Very specific feature which is used by async compute samples.
     * @param enable If true, present queue will have prio 1.0 and other queues have prio 0.5.
     * Default state is false, where all queues have 0.5 priority.
     */
    void set_high_priority_graphics_queue_enable(bool enable)
    {
        high_priority_graphics_queue = enable;
    }

    /**
     * @brief Returns high priority graphics queue state.
     * @return High priority state.
     */
    [[nodiscard]] bool has_high_priority_graphics_queue() const
    {
        return high_priority_graphics_queue;
    }

private:
    // Handle to the Vulkan instance
    Instance& instance;
    // Handle to the Vulkan physical device
    vk::PhysicalDevice handle{nullptr};
    // The features that this GPU supports
    vk::PhysicalDeviceFeatures features;
    // The extensions that this GPU supports
    std::vector<vk::ExtensionProperties> device_extensions;
    // The GPU properties
    vk::PhysicalDeviceProperties properties;
    // The GPU memory properties
    vk::PhysicalDeviceMemoryProperties memory_properties;
    // The GPU queue family properties
    std::vector<vk::QueueFamilyProperties> queue_family_properties;
    // The features that will be requested to be enabled in the logical device
    vk::PhysicalDeviceFeatures requested_features;
    // The extension feature pointer
    void* last_requested_extension_feature{nullptr};
    // Holds the extension feature structures, we use a map to retain an order of requested structures
    std::map<vk::StructureType, std::shared_ptr<void>> extension_features;
    bool high_priority_graphics_queue{false};
};

#define REQUEST_OPTIONAL_FEATURE(gpu, Feature, flag) gpu.request_optional_feature<Feature>(&Feature::flag, #Feature, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, Feature, flag) gpu.request_required_feature<Feature>(&Feature::flag, #Feature, #flag)
} // portal
