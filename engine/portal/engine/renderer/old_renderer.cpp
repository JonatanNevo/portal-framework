
//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "old_renderer.h"

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <tracy/Tracy.hpp>

#include "portal/core/files/file_system.h"
#include "vulkan_utils.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{
static auto logger = Log::get_logger("Renderer");


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const auto MAX_FRAMES_IN_FLIGHT = 2;

static std::string MODEL_PATH = "resources/viking_room.obj";
static std::string TEXTURE_PATH = "resources/viking_room.png";

#ifdef PORTAL_DEBUG
constexpr bool enable_validation_layers = true;
#else
constexpr bool enable_validation_layers = false;
#endif


static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* data
    )
{
    auto level = Log::LogLevel::Info;
    switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity))
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        level = Log::LogLevel::Debug;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        level = Log::LogLevel::Info;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        level = Log::LogLevel::Warn;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        level = Log::LogLevel::Error;
        break;
    }
    logger->log(SOURCE_LOC, static_cast<spdlog::level::level_enum>(level), pCallbackData->pMessage);
    return vk::False;
}


static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT extension,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* data
    )
{
    return debug_callback(
        static_cast<VkDebugUtilsMessageSeverityFlagBitsEXT>(severity),
        static_cast<VkDebugUtilsMessageTypeFlagsEXT>(extension),
        reinterpret_cast<const VkDebugUtilsMessengerCallbackDataEXT*>(pCallbackData),
        data
        );
}


bool has_stencil_component(const vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

vk::VertexInputBindingDescription Vertex::get_binding_description()
{
    return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
}

std::array<vk::VertexInputAttributeDescription, 3> Vertex::get_attribute_descriptions()
{
    return {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord))
    };
}


bool Vertex::operator==(const Vertex& other) const
{
    return position == other.position && color == other.color && tex_coord == other.tex_coord;
}

void OldRenderer::frame_buffer_size_callback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
    const auto renderer = static_cast<OldRenderer*>(glfwGetWindowUserPointer(window));
    renderer->frame_buffer_resized = true;

}

glm::mat4 GameObject::get_model_matrix() const
{
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model *= glm::mat4_cast(rotation);
    model = glm::scale(model, scale);
    return model;
}

void OldRenderer::run()
{
    try
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }
    catch (vk::Error& e)
    {
        LOGGER_FATAL("Caught Vulkan error: {}", e.what());
    }
}

void OldRenderer::init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Portal Engine", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
}

void OldRenderer::init_vulkan()
{
    create_instance();
    create_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_command_pool();
    create_depth_resources();
    create_color_resources();
    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();
    load_model();
    setup_game_objects();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();

    tracy_context = TracyVkContext(*physical_device, *device, *graphics_queue, *command_buffers[0]);
}

void OldRenderer::main_loop()
{
    timer.start();
    while (!glfwWindowShouldClose(window))
    {
        FrameMark;
        delta_time = timer.tick<Timer::Seconds>();
        glfwPollEvents();
        draw_frame(delta_time);
    }
    device.waitIdle();
}

void OldRenderer::cleanup()
{
    cleanup_swap_chain();

    if (tracy_context)
    {
        TracyVkDestroy(tracy_context);
        tracy_context = nullptr;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

void OldRenderer::setup_game_objects()
{
    objects.resize(3);

    // Object 1 - Center
    objects[0].position = {0.0f, 0.0f, 0.0f};
    objects[0].rotation = glm::quat({0, 0, 0});
    objects[0].scale = {1.0f, 1.0f, 1.0f};

    // Object 2 - Left
    objects[1].position = {-2.0f, 0.0f, -1.0f};
    objects[1].rotation = glm::quat({0.0f, glm::radians(45.0f), 0.0f});
    objects[1].scale = {0.75f, 0.75f, 0.75f};

    // Object 3 - Right
    objects[2].position = {2.0f, 0.0f, -1.0f};
    objects[2].rotation = glm::quat({0.0f, glm::radians(-45.0f), 0.0f});
    objects[2].scale = {0.75f, 0.75f, 0.75f};
}

void OldRenderer::create_vma_allocator()
{
    // VmaVulkanFunctions vulkan_functions = {};
    // vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    // vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    //
    // VmaAllocatorCreateInfo allocator_create_info = {
    //     .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
    //     .physicalDevice = *physical_device,
    //     .device = *device,
    //     .pVulkanFunctions = &vulkan_functions,
    //     .instance = *instance,
    //     .vulkanApiVersion = VK_API_VERSION_1_4
    // };
    // vmaCreateAllocator(&allocator_create_info, &vma_allocator);
}

void OldRenderer::create_instance()
{
    constexpr vk::ApplicationInfo app_info = {
        .pApplicationName = "Portal Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Portal Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    auto required_layers = get_required_validation_layers();
    auto required_extensions = get_required_extensions();

    const vk::InstanceCreateInfo create_info = {
#ifdef PORTAL_PLATFORM_MACOS
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames = required_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()
    };
    instance = context.createInstance(create_info);
}

void OldRenderer::create_debug_messenger()
{
    if (!enable_validation_layers)
        return;

    constexpr auto severity_flags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    constexpr auto message_type_flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

    constexpr vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create{
        .messageSeverity = severity_flags,
        .messageType = message_type_flags,
        .pfnUserCallback = &debug_callback,
    };
    debug_messenger = instance.createDebugUtilsMessengerEXT(debug_utils_create);
}

void OldRenderer::create_surface()
{
    VkSurfaceKHR surface_handle;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surface_handle) != VK_SUCCESS)
    {
        LOGGER_ERROR("Failed to create window surface!");
        throw std::runtime_error("Failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, surface_handle);
}

void OldRenderer::pick_physical_device()
{
    const auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        LOGGER_ERROR("No Vulkan physical devices found!");
        throw std::runtime_error("No Vulkan physical devices found!");
    }

    std::multimap<uint32_t, vk::raii::PhysicalDevice> candidates;
    for (const auto& dev : devices)
    {
        uint32_t score = rate_device_suitability(dev);
        candidates.insert(std::make_pair(score, dev));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0)
    {
        physical_device = candidates.rbegin()->second;
        msaa_samples = get_max_usable_sample_count();
    }
    else
        throw std::runtime_error("No suitable gpu found!");

    LOGGER_INFO("Picked GPU: {}", physical_device.getProperties().deviceName.data());
}

void OldRenderer::create_logical_device()
{
    auto graphics_queue_family_index = find_queue_families(physical_device, vk::QueueFlagBits::eGraphics);

    auto queue_families = physical_device.getQueueFamilyProperties();
    auto present_queue_family_index = physical_device.getSurfaceSupportKHR(graphics_queue_family_index, *surface)
        ? graphics_queue_family_index
        : queue_families.size();
    if (present_queue_family_index == queue_families.size())
    {
        // the graphicsIndex doesn't support present -> look for another family index that supports both
        // graphics and present
        for (size_t i = 0; i < queue_families.size(); i++)
        {
            if ((queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
            {
                graphics_queue_family_index = static_cast<uint32_t>(i);
                present_queue_family_index = graphics_queue_family_index;
                break;
            }
        }
        if (present_queue_family_index == queue_families.size())
        {
            // there's nothing like a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for (size_t i = 0; i < queue_families.size(); i++)
            {
                if (physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
                {
                    present_queue_family_index = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((graphics_queue_family_index == queue_families.size()) || (present_queue_family_index == queue_families.size()))
    {
        LOGGER_ERROR("Could not find a queue family that supports graphics or present");
        throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    float queue_priority = 0.0f;
    vk::DeviceQueueCreateInfo queue_create_info = {
        .queueFamilyIndex = graphics_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };


    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        feature_chain = {
            {.features = {.sampleRateShading = true, .samplerAnisotropy = true}},
            {.synchronization2 = true, .dynamicRendering = true},
            {.extendedDynamicState = true}
        };

    vk::DeviceCreateInfo create_info = {
        .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(vulkan::g_device_extensions.size()),
        .ppEnabledExtensionNames = vulkan::g_device_extensions.data(),
    };
    device = physical_device.createDevice(create_info);
    graphics_queue = device.getQueue(graphics_queue_family_index, 0);
    present_queue = device.getQueue(static_cast<uint32_t>(present_queue_family_index), 0);
}

void OldRenderer::create_swap_chain()
{
    const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
    const auto [format, colorSpace] = choose_surface_format(physical_device.getSurfaceFormatsKHR(*surface));
    swap_chain_extent = choose_extent(surface_capabilities);
    const auto min_image_count = std::min(std::max(3u, surface_capabilities.minImageCount), surface_capabilities.maxImageCount);

    const vk::SwapchainCreateInfoKHR swap_chain_create_info = {
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface,
        .minImageCount = min_image_count,
        .imageFormat = format,
        .imageColorSpace = colorSpace,
        .imageExtent = swap_chain_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = choose_present_mode(physical_device.getSurfacePresentModesKHR(surface)),
        .clipped = true,
        .oldSwapchain = nullptr
    };
    swap_chain = device.createSwapchainKHR(swap_chain_create_info);
    swap_chain_images = swap_chain.getImages();
    swap_chain_image_format = format;
}

void OldRenderer::create_image_views()
{
    swap_chain_image_views.clear();
    for (const auto image : swap_chain_images)
    {
        swap_chain_image_views.emplace_back(create_image_view(image, swap_chain_image_format, vk::ImageAspectFlagBits::eColor));
    }
}

void OldRenderer::create_descriptor_set_layout()
{
    std::array bindings = {
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
    };

    const vk::DescriptorSetLayoutCreateInfo create_info = {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    descriptor_set_layout = device.createDescriptorSetLayout(create_info);
}

void OldRenderer::create_graphics_pipeline()
{
    // TODO: use resources
    const auto shader_code = portal::FileSystem::read_file_binary("triangle.shading.slang.spv");
    const auto module = create_shader_module(shader_code);

    const vk::PipelineShaderStageCreateInfo vert_shader_stage_info{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = module,
        .pName = "vertMain"
    };

    const vk::PipelineShaderStageCreateInfo frag_shader_stage_info{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = module,
        .pName = "fragMain"
    };

    vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    std::vector dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount = 1,
        .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = msaa_samples,
        .sampleShadingEnable = vk::True,
        .minSampleShading = 0.2f // min fraction for sample shading; closer to one is smoother
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo color_blending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_create_info{
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptor_set_layout,
        .pushConstantRangeCount = 0
    };
    pipeline_Layout = device.createPipelineLayout(pipeline_layout_create_info);

    auto depth_format = find_depth_format();

    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swap_chain_image_format,
        .depthAttachmentFormat = depth_format,
    };

    vk::GraphicsPipelineCreateInfo pipeline_info{
        .pNext = &pipeline_rendering_create_info,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = pipeline_Layout,
        .renderPass = nullptr
    };

    graphics_pipeline = device.createGraphicsPipeline(nullptr, pipeline_info);
}

void OldRenderer::create_command_pool()
{
    const auto graphics_index = find_queue_families(physical_device, vk::QueueFlagBits::eGraphics);
    const vk::CommandPoolCreateInfo pool_info{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_index
    };
    command_pool = device.createCommandPool(pool_info);
}

void OldRenderer::create_depth_resources()
{
    const auto depth_format = find_depth_format();
    create_image(
        swap_chain_extent.width,
        swap_chain_extent.height,
        depth_format,
        1,
        msaa_samples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        depth_image,
        depth_image_memory
        );
    depth_image_view = create_image_view(depth_image, depth_format, vk::ImageAspectFlagBits::eDepth);
}

void OldRenderer::create_texture_image()
{
    // stbimage
    int width, height, channels;
    const auto image_data = portal::FileSystem::read_file_binary(TEXTURE_PATH);
    stbi_uc* pixels = stbi_load_from_memory(image_data.as<uint8_t*>(), static_cast<int>(image_data.size), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    mip_levels = static_cast<uint32_t>(std::floor(std::log2(glm::max(width, height)))) + 1;
    auto size = width * height * 4;

    vk::raii::Buffer staging_buffer = nullptr;
    vk::raii::DeviceMemory staging_memory = nullptr;
    create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer,
        staging_memory
        );

    void* data = staging_memory.mapMemory(0, size);
    std::memcpy(data, pixels, size);
    staging_memory.unmapMemory();

    vk::Format texture_format = vk::Format::eR8G8B8A8Srgb; // Default format, should be determined from KTX metadata

    stbi_image_free(pixels);

    create_image(
        width,
        height,
        texture_format,
        mip_levels,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        texture,
        texture_memory
        );

    auto command_buffer = begin_single_time_commands();
    transition_image_layout(command_buffer, texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mip_levels);
    copy_buffer_to_image(command_buffer, staging_buffer, texture, width, height);
    // transition_image_layout(command_buffer, texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, mip_levels);
    // TODO: load mipmaps from ktx texture https://docs.vulkan.org/tutorial/latest/15_GLTF_KTX2_Migration.html
    generate_mipmaps(command_buffer, texture, texture_format, width, height, mip_levels);
    end_single_time_commands(command_buffer);

    // ktx
    // ktxTexture* k_texture;
    // KTX_error_code result = ktxTexture_CreateFromNamedFile(
    //     TEXTURE_PATH.c_str(),
    //     KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
    //     &k_texture
    //     );
    //
    // if (result != KTX_SUCCESS) {
    //     throw std::runtime_error("failed to load ktx texture image!");
    // }
    //
    // uint32_t width = k_texture->baseWidth;
    // uint32_t height = k_texture->baseHeight;
    // ktx_size_t size = ktxTexture_GetImageSize(k_texture, 0);
    // ktx_uint8_t* ktx_texture_data = ktxTexture_GetData(k_texture);
    // mip_levels = static_cast<uint32_t>(std::floor(std::log2(glm::max(width, height)))) + 1;
    //
    // vk::raii::Buffer staging_buffer = nullptr;
    // vk::raii::DeviceMemory staging_memory = nullptr;
    // create_buffer(
    // size,
    // vk::BufferUsageFlagBits::eTransferSrc,
    // vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
    // staging_buffer,
    // staging_memory
    // );
    //
    // void* data = staging_memory.mapMemory(0, size);
    // std::memcpy(data, ktx_texture_data, size);
    // staging_memory.unmapMemory();
    //
    // // Determine the Vulkan format from KTX format
    // vk::Format texture_format = vk::Format::eR8G8B8A8Srgb; // Default format, should be determined from KTX metadata
    //
    // create_image(
    //     width,
    //     height,
    //     texture_format,
    //     mip_levels,
    //     vk::SampleCountFlagBits::e1,
    //     vk::ImageTiling::eOptimal,
    //     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    //     vk::MemoryPropertyFlagBits::eDeviceLocal,
    //     texture,
    //     texture_memory
    //     );
    //
    // auto command_buffer = begin_single_time_commands();
    // transition_image_layout(command_buffer, texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mip_levels);
    // copy_buffer_to_image(command_buffer, staging_buffer, texture, width, height);
    // // transition_image_layout(command_buffer, texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, mip_levels);
    // // TODO: load mipmaps from ktx texture https://docs.vulkan.org/tutorial/latest/15_GLTF_KTX2_Migration.html
    // generate_mipmaps(command_buffer, texture, texture_format, width, height, mip_levels);
    // end_single_time_commands(command_buffer);
    //
    // // Cleanup KTX resources
    // ktxTexture_Destroy(k_texture);
}

void OldRenderer::create_texture_image_view()
{
    texture_image_view = create_image_view(texture, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mip_levels);
}

void OldRenderer::create_texture_sampler()
{
    const auto properties = physical_device.getProperties();
    const vk::SamplerCreateInfo sampler_info{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False,
    };

    texture_sampler = device.createSampler(sampler_info);
}


void OldRenderer::load_model()
{
    // load obj
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (!uniqueVertices.contains(vertex))
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    // glft
    // tinygltf::Model model;
    // tinygltf::TinyGLTF loader;
    //
    // std::string err;
    // std::string warn;
    // bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, MODEL_PATH);
    // if (!warn.empty())
    // {
    //     LOGGER_WARN("glTF warning: {}", warn);
    // }
    // if (!err.empty())
    // {
    //     LOGGER_ERROR("glTF error: {}", err);
    // }
    // if (!ret)
    // {
    //     LOGGER_ERROR("Failed to load glTF model from file: {}", MODEL_PATH);
    //     throw std::runtime_error("Failed to load glTF model");
    // }
    //
    // std::unordered_map<Vertex, uint32_t> unique_vertices{};
    // for (const auto& mesh : model.meshes)
    // {
    //     for (const auto& primitive : mesh.primitives)
    //     {
    //         const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];
    //         const tinygltf::BufferView& index_buffer_view = model.bufferViews[index_accessor.bufferView];
    //         const tinygltf::Buffer& index_buffer = model.buffers[index_buffer_view.buffer];
    //
    //         const tinygltf::Accessor& position_accessor = model.accessors[primitive.attributes.at("POSITION")];
    //         const tinygltf::BufferView& position_buffer_view = model.bufferViews[position_accessor.bufferView];
    //         const tinygltf::Buffer& position_buffer = model.buffers[position_buffer_view.buffer];
    //
    //         const bool has_tex_coords = primitive.attributes.contains("TEXCOORD_0");
    //         const tinygltf::Accessor* tex_coords_accessor = nullptr;
    //         const tinygltf::BufferView* tex_coords_buffer_view = nullptr;
    //         const tinygltf::Buffer* tex_coords_buffer = nullptr;
    //
    //         if (has_tex_coords)
    //         {
    //             tex_coords_accessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
    //             tex_coords_buffer_view = &model.bufferViews[tex_coords_accessor->bufferView];
    //             tex_coords_buffer = &model.buffers[tex_coords_buffer_view->buffer];
    //         }
    //
    //         // Process vertices
    //         for (size_t i = 0; i < position_accessor.count; i++)
    //         {
    //             Vertex vertex{};
    //             auto* pos = reinterpret_cast<const float*>(&position_buffer.data[position_buffer_view.byteOffset + position_accessor.byteOffset + i *
    //                 12]);
    //             vertex.position = {pos[0], pos[1], pos[2]};
    //
    //             // Get texture coordinates if available
    //             if (has_tex_coords)
    //             {
    //                 auto* texCoord = reinterpret_cast<const float*>(&tex_coords_buffer->data[tex_coords_buffer_view->byteOffset + tex_coords_accessor
    //                     ->byteOffset + i * 8]);
    //                 vertex.tex_coord = {texCoord[0], 1.0f - texCoord[1]};
    //             }
    //             else
    //             {
    //                 vertex.tex_coord = {0.0f, 0.0f};
    //             }
    //
    //             vertex.color = {1.0f, 1.0f, 1.0f};
    //
    //             // Add vertex if unique
    //             if (!unique_vertices.contains(vertex))
    //             {
    //                 unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
    //                 vertices.push_back(vertex);
    //             }
    //         }
    //
    //         // Process indices
    //         const unsigned char* index_data = &index_buffer.data[index_buffer_view.byteOffset + index_accessor.byteOffset];
    //
    //         // Handle different index component types
    //         if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    //         {
    //             const auto* indices16 = reinterpret_cast<const uint16_t*>(index_data);
    //             for (size_t i = 0; i < index_accessor.count; i++)
    //             {
    //                 Vertex vertex = vertices[indices16[i]];
    //                 indices.push_back(unique_vertices[vertex]);
    //             }
    //         }
    //         else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    //         {
    //             const auto* indices32 = reinterpret_cast<const uint32_t*>(index_data);
    //             for (size_t i = 0; i < index_accessor.count; i++)
    //             {
    //                 Vertex vertex = vertices[indices32[i]];
    //                 indices.push_back(unique_vertices[vertex]);
    //             }
    //         }
    //         else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
    //         {
    //             const auto* indices8 = reinterpret_cast<const uint8_t*>(index_data);
    //             for (size_t i = 0; i < index_accessor.count; i++)
    //             {
    //                 Vertex vertex = vertices[indices8[i]];
    //                 indices.push_back(unique_vertices[vertex]);
    //             }
    //         }
    //     }
    // }
}

void OldRenderer::create_vertex_buffer()
{
    const auto size = sizeof(vertices[0]) * vertices.size();
    vk::raii::Buffer staging_buffer = nullptr;
    vk::raii::DeviceMemory staging_buffer_memory = nullptr;
    create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer,
        staging_buffer_memory
        );

    void* data = staging_buffer_memory.mapMemory(0, size);
    std::memcpy(data, vertices.data(), size);
    staging_buffer_memory.unmapMemory();

    create_buffer(
        size,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertex_buffer,
        vertex_buffer_memory
        );

    auto command_buffer = begin_single_time_commands();
    copy_buffer(command_buffer, staging_buffer, vertex_buffer, size);
    end_single_time_commands(command_buffer);
}

void OldRenderer::create_index_buffer()
{
    auto size = sizeof(indices[0]) * indices.size();
    vk::raii::Buffer staging_buffer = nullptr;
    vk::raii::DeviceMemory staging_buffer_memory = nullptr;
    create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer,
        staging_buffer_memory
        );

    void* data = staging_buffer_memory.mapMemory(0, size);
    std::memcpy(data, indices.data(), size);
    staging_buffer_memory.unmapMemory();

    create_buffer(
        size,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        index_buffer,
        index_buffer_memory
        );

    auto command_buffer = begin_single_time_commands();
    copy_buffer(command_buffer, staging_buffer, index_buffer, size);
    end_single_time_commands(command_buffer);
}

void OldRenderer::create_uniform_buffers()
{
    for (auto& object : objects)
    {
        object.uniform_buffers.clear();
        object.uniform_buffers_memory.clear();
        object.uniform_buffers_mapped_ptr.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            auto size = sizeof(UniformBufferObject);
            vk::raii::Buffer buffer = nullptr;
            vk::raii::DeviceMemory buffer_memory = nullptr;
            create_buffer(
                size,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                buffer,
                buffer_memory
                );
            object.uniform_buffers.emplace_back(std::move(buffer));
            object.uniform_buffers_memory.emplace_back(std::move(buffer_memory));
            object.uniform_buffers_mapped_ptr.emplace_back(object.uniform_buffers_memory.back().mapMemory(0, size));
        }
    }
}

void OldRenderer::create_descriptor_pool()
{
    std::array pool_size = {
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * objects.size())
        },
        vk::DescriptorPoolSize{
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * objects.size())
        }
    };

    const vk::DescriptorPoolCreateInfo pool_info = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * objects.size()),
        .poolSizeCount = static_cast<uint32_t>(pool_size.size()),
        .pPoolSizes = pool_size.data()
    };
    descriptor_pool = device.createDescriptorPool(pool_info);
}

void OldRenderer::create_descriptor_sets()
{
    for (auto& object : objects)
    {
        object.descriptor_sets.clear();
        std::vector layouts(MAX_FRAMES_IN_FLIGHT, *descriptor_set_layout);
        vk::DescriptorSetAllocateInfo alloc_info = {
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };
        object.descriptor_sets = device.allocateDescriptorSets(alloc_info);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DescriptorBufferInfo buffer_info{
                .buffer = object.uniform_buffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };
            vk::DescriptorImageInfo image_info{
                .sampler = texture_sampler,
                .imageView = texture_image_view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };
            std::array descriptor_writes = {
                vk::WriteDescriptorSet{
                    .dstSet = object.descriptor_sets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &buffer_info
                },
                vk::WriteDescriptorSet{
                    .dstSet = object.descriptor_sets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo = &image_info
                }
            };
            device.updateDescriptorSets(descriptor_writes, {});
        }
    }
}

void OldRenderer::create_command_buffers()
{
    command_buffers.clear();
    const vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    command_buffers = vk::raii::CommandBuffers(device, alloc_info);
}

void OldRenderer::create_sync_objects()
{
    present_complete_semaphores.clear();
    render_finished_semaphores.clear();
    in_flight_fences.clear();

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        present_complete_semaphores.emplace_back(device, vk::SemaphoreCreateInfo());
        render_finished_semaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        in_flight_fences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void OldRenderer::create_color_resources()
{
    const vk::Format color_format = swap_chain_image_format;
    create_image(
        swap_chain_extent.width,
        swap_chain_extent.height,
        color_format,
        1,
        msaa_samples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        color_image,
        color_image_memory
        );
    color_image_view = create_image_view(color_image, color_format, vk::ImageAspectFlagBits::eColor);
}

void OldRenderer::record_command_buffer(uint32_t image_index)
{
    command_buffers[current_frame].begin({});

    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        image_index,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );

    // Transition the multisampled color image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        command_buffers[current_frame],
        color_image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor,
        1
        );

    // Transition depth image to depth attachment optimal layout
    transition_image_layout(
        command_buffers[current_frame],
        depth_image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        vk::ImageAspectFlagBits::eDepth,
        1
        );
    constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.0f, 0);


    // Color attachment (multisampled) with resolve attachment
    vk::RenderingAttachmentInfo color_attachment_info = {
        .imageView = color_image_view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = swap_chain_image_views[image_index],
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear_color
    };

    vk::RenderingAttachmentInfo depth_attachment_info = {
        .imageView = depth_image_view,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clear_depth
    };

    vk::RenderingInfo rendering_info = {
        .renderArea = {.offset = {0, 0}, .extent = swap_chain_extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = &depth_attachment_info,
    };

    command_buffers[current_frame].beginRendering(rendering_info);

    command_buffers[current_frame].setViewport(
        0,
        vk::Viewport(
            0.0f,
            0.0f,
            static_cast<float>(swap_chain_extent.width),
            static_cast<float>(swap_chain_extent.height),
            0.0f,
            1.0f
            )
        );
    command_buffers[current_frame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swap_chain_extent));

    command_buffers[current_frame].bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);

    // Bind vertex and index buffers (shared by all objects)
    command_buffers[current_frame].bindVertexBuffers(0, *vertex_buffer, {0});
    command_buffers[current_frame].bindIndexBuffer(*index_buffer, 0, vk::IndexType::eUint32);

    // Draw each object with its own descriptor set
    for (const auto& object : objects)
    {
        // Bind the descriptor set for this object
        command_buffers[current_frame].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            *pipeline_Layout,
            0,
            *object.descriptor_sets[current_frame],
            nullptr
            );

        // Draw the object
        command_buffers[current_frame].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    command_buffers[current_frame].endRendering();

    // After rendering, transition the swapchain image to PRESENT_SRC
    transition_image_layout(
        image_index,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
        );

    command_buffers[current_frame].end();
}

void OldRenderer::update_uniform_buffer([[maybe_unused]] const float dt, const uint32_t frame_index)
{
    // Camera and projection matrices (shared by all objects)
    const auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    auto projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(swap_chain_extent.width) / static_cast<float>(swap_chain_extent.height),
        0.1f,
        10.0f
        );

    // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
    // The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
    // If you don’t do this, then the image will be rendered upside down.
    projection[1][1] *= -1;

    // Update uniform buffers for each object
    for (auto& object : objects)
    {
        // Apply continuous rotation to the object
        object.rotation = glm::rotate(object.rotation, dt * glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // Create and update the UBO
        UniformBufferObject ubo{
            .model = object.get_model_matrix(),
            .view = view,
            .projection = projection
        };

        // Copy the UBO data to the mapped memory
        memcpy(object.uniform_buffers_mapped_ptr[frame_index], &ubo, sizeof(ubo));
    }
}

void OldRenderer::draw_frame(const float dt)
{
    while (vk::Result::eTimeout == device.waitForFences(*in_flight_fences[current_frame], vk::True, UINT64_MAX));

    auto [result, image_index] = swap_chain.acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        *present_complete_semaphores[semaphore_index],
        nullptr
        );

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreate_swap_chain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device.resetFences(*in_flight_fences[current_frame]);
    command_buffers[current_frame].reset();
    record_command_buffer(image_index);

    update_uniform_buffer(dt, current_frame);

    vk::PipelineStageFlags wait_destination_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*present_complete_semaphores[semaphore_index],
        .pWaitDstStageMask = &wait_destination_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffers[current_frame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*render_finished_semaphores[image_index],
    };
    graphics_queue.submit(submit_info, *in_flight_fences[current_frame]);

    const vk::PresentInfoKHR present_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*render_finished_semaphores[image_index],
        .swapchainCount = 1,
        .pSwapchains = &*swap_chain,
        .pImageIndices = &image_index
    };

    try
    {
        result = present_queue.presentKHR(present_info);
        if (result == vk::Result::eSuboptimalKHR || frame_buffer_resized)
        {
            frame_buffer_resized = false;
            recreate_swap_chain();
        }
    }
    catch (vk::OutOfDateKHRError&)
    {
        frame_buffer_resized = false;
        recreate_swap_chain();
    }
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    semaphore_index = (semaphore_index + 1) % present_complete_semaphores.size();
}

void OldRenderer::cleanup_swap_chain()
{
    swap_chain_image_views.clear();
    swap_chain = nullptr;
}

void OldRenderer::recreate_swap_chain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    LOGGER_DEBUG("Recreating swap chain to {}x{}", width, height);

    device.waitIdle();

    cleanup_swap_chain();
    create_swap_chain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
}

void OldRenderer::create_buffer(
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    vk::raii::Buffer& buffer,
    vk::raii::DeviceMemory& buffer_memory
    ) const
{
    const vk::BufferCreateInfo buffer_create_info{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    buffer = device.createBuffer(buffer_create_info);
    const auto memory_requirements = buffer.getMemoryRequirements();
    const vk::MemoryAllocateInfo memory_allocation_info{
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties)
    };
    buffer_memory = device.allocateMemory(memory_allocation_info);
    buffer.bindMemory(buffer_memory, 0);
}

void OldRenderer::create_image(
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const uint32_t mip_level,
    const vk::SampleCountFlagBits samples,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    vk::raii::Image& image,
    vk::raii::DeviceMemory& imageMemory
    ) const
{
    vk::ImageCreateInfo image_create_info{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = vk::Extent3D(width, height, 1),
        .mipLevels = mip_level,
        .arrayLayers = 1,
        .samples = samples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    image = device.createImage(image_create_info);

    const auto memory_requirements = image.getMemoryRequirements();
    const vk::MemoryAllocateInfo memory_allocation_info{
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties)
    };
    imageMemory = device.allocateMemory(memory_allocation_info);
    image.bindMemory(imageMemory, 0);
}

vk::raii::ImageView OldRenderer::create_image_view(
    const vk::Image& image,
    const vk::Format format,
    const vk::ImageAspectFlags aspect_flags,
    uint32_t mip_level
    ) const
{
    const vk::ImageViewCreateInfo image_view_create_info = {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {aspect_flags, 0, mip_level, 0, 1},
    };
    return device.createImageView(image_view_create_info);
}

void OldRenderer::generate_mipmaps(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::raii::Image& image,
    const vk::Format image_format,
    const int32_t width,
    const int32_t height,
    const uint32_t mip_level
    ) const
{
    const auto format_properties = physical_device.getFormatProperties(image_format);
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mip_width = width;
    int32_t mip_height = height;

    const vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    for (uint32_t i = 1; i < mip_level; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
        command_buffer.pipelineBarrier2(dependency_info);

        vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dst_offsets;
        offsets[0] = vk::Offset3D(0, 0, 0);
        offsets[1] = vk::Offset3D(mip_width, mip_height, 1);
        dst_offsets[0] = vk::Offset3D(0, 0, 0);
        dst_offsets[1] = vk::Offset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1);
        vk::ImageBlit blit = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
            .srcOffsets = offsets,
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1},
            .dstOffsets = dst_offsets
        };
        command_buffer.blitImage(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            {blit},
            vk::Filter::eLinear
            );

        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        command_buffer.pipelineBarrier2(dependency_info);

        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
    }

    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        barrier.subresourceRange.baseMipLevel = mip_level - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    command_buffer.pipelineBarrier2(dependency_info);
}

void OldRenderer::copy_buffer(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::raii::Buffer& src,
    const vk::raii::Buffer& dst,
    const vk::DeviceSize size
    ) const
{
    command_buffer.copyBuffer(src, dst, vk::BufferCopy(0, 0, size));
}

void OldRenderer::copy_buffer_to_image(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::raii::Buffer& buffer,
    const vk::raii::Image& image,
    uint32_t width,
    uint32_t height
    ) const
{
    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent{width, height, 1}
    };
    command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
}

vk::raii::CommandBuffer OldRenderer::begin_single_time_commands() const
{
    const vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer command_buffer = std::move(device.allocateCommandBuffers(alloc_info).front());

    command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    return command_buffer;
}

void OldRenderer::end_single_time_commands(const vk::raii::CommandBuffer& command_buffer) const
{
    command_buffer.end();

    graphics_queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*command_buffer}, nullptr);
    graphics_queue.waitIdle();
}

vk::Format OldRenderer::find_depth_format() const
{
    return find_supported_format(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
}

vk::Format OldRenderer::find_supported_format(
    const std::vector<vk::Format>& candidates,
    const vk::ImageTiling tiling,
    const vk::FormatFeatureFlags features
    ) const
{
    for (const auto format : candidates)
    {
        vk::FormatProperties props = physical_device.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

uint32_t OldRenderer::rate_device_suitability(const vk::raii::PhysicalDevice& device)
{
    uint32_t score = 0;
    const auto properties = device.getProperties();
    const auto features = device.getFeatures();
    const auto queue_families = device.getQueueFamilyProperties();

    if (properties.apiVersion < vk::ApiVersion14)
        return 0;

    if (std::ranges::find_if(
        queue_families,
        [](const auto& prop)
        {
            return (prop.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        }
        ) == queue_families.end())
    {
        // No graphics queue found
        return 0;
    }

    auto extensions = device.enumerateDeviceExtensionProperties();
    for (const auto& extension : vulkan::g_device_extensions)
    {
        if (std::ranges::find_if(extensions, [extension](auto const& ext) { return strcmp(ext.extensionName, extension) == 0; }) == extensions.end())
        {
            return 0;
        }
    }

    if (!features.samplerAnisotropy)
        return 0;

    // Discrete GPUs have a significant performance advantage
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
    {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += properties.limits.maxImageDimension2D;

    LOGGER_DEBUG("Available Device: {}", properties.deviceName.data());
    return score;
}

std::vector<const char*> OldRenderer::get_required_extensions() const
{
    // Get the required instance extensions from GLFW.
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // Check if the required GLFW extensions are supported by the Vulkan implementation.
    auto extension_properties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; ++i)
    {
        if (std::ranges::none_of(
            extension_properties,
            [extension=glfw_extensions[i]](const auto& property)
            {
                return strcmp(property.extensionName, extension) == 0;
            }
            ))
        {
            LOG_DEBUG_TAG("Renderer", "Required GLFW extension not supported: {}", glfw_extensions[i]);
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_extensions[i]));
        }
    }

    std::vector<const char*> extensions = {glfw_extensions, glfw_extensions + glfw_extension_count};
#ifdef PORTAL_PLATFORM_MACOS
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif
    if (enable_validation_layers)
        extensions.push_back(vk::EXTDebugUtilsExtensionName);


#ifdef PORTAL_DEBUG
    {
        const auto extension_props = context.enumerateInstanceExtensionProperties();
        LOG_TRACE_TAG("Renderer", "Available instance extensions:");
        for (const auto& [extensionName, specVersion] : extension_props)
        {
            std::string extension_name = extensionName;
            auto has_extension = std::ranges::find(extensions, extension_name) != extensions.end() ? "x" : " ";
            LOG_TRACE_TAG("Renderer", "  {} {}", has_extension, extension_name);
        }
    }
    LOG_TRACE_TAG("Renderer", "");
#endif
    return extensions;
}

std::vector<const char*> OldRenderer::get_required_validation_layers() const
{
    // Get required validation layers
    std::vector<const char*> required_layers = {};
    if (enable_validation_layers)
        required_layers.assign(vulkan::g_validation_layers.begin(), vulkan::g_validation_layers.end());

    // Check if the required layers are supported by the Vulkan implementation.
    const auto layer_properties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(
        required_layers,
        [&layer_properties](const auto& required_layer)
        {
            return std::ranges::none_of(
                layer_properties,
                [required_layer](const auto& layer_property)
                {
                    return strcmp(layer_property.layerName, required_layer) == 0;
                }
                );
        }
        ))
    {
        LOG_DEBUG_TAG("Renderer", "One or more required layers are not supported!");
        throw std::runtime_error("One or more required layers are not supported!");
    }


#ifdef PORTAL_DEBUG
    {
        const auto layers_prop = context.enumerateInstanceLayerProperties();
        LOG_TRACE_TAG("Renderer", "Available instance layers:");
        for (const auto& layer : layers_prop)
        {
            std::string layer_name = layer.layerName;
            auto has_layer = std::ranges::find(required_layers, layer_name) != required_layers.end() ? "v" : " ";
            LOG_TRACE_TAG("Renderer", "  {} {}", has_layer, layer_name);
        }
    }
    LOG_TRACE_TAG("Renderer", "");
#endif
    return required_layers;
}

uint32_t OldRenderer::find_queue_families(const vk::raii::PhysicalDevice& device, vk::QueueFlagBits queue_type)
{
    auto families = device.getQueueFamilyProperties();
    // get the first index into queueFamilyProperties which supports graphics
    const auto graphics_queue_family_prop =
        std::ranges::find_if(
            families,
            [queue_type](const auto& prop)
            {
                return (prop.queueFlags & queue_type) != static_cast<vk::QueueFlags>(0);
            }
            );

    return static_cast<uint32_t>(std::distance(families.begin(), graphics_queue_family_prop));
}

vk::SurfaceFormatKHR OldRenderer::choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats)
{
    for (auto& [format, color_space] : available_formats)
    {
        if (format == vk::Format::eB8G8R8A8Srgb && color_space == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return {format, color_space};
        }
    }
    return available_formats[0];
}

vk::PresentModeKHR OldRenderer::choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes)
{
    for (const auto& present_mode : available_present_modes)
    {
        if (present_mode == vk::PresentModeKHR::eMailbox)
            return present_mode;
    }
    // FIFO is guaranteed to be supported
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D OldRenderer::choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t OldRenderer::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) const
{
    const auto memory_properties = physical_device.getMemoryProperties();
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

vk::raii::ShaderModule OldRenderer::create_shader_module(const Buffer& code) const
{
    const vk::ShaderModuleCreateInfo create_info{
        .codeSize = code.size * sizeof(char),
        .pCode = code.as<uint32_t*>()
    };
    return device.createShaderModule(create_info);
}

vk::SampleCountFlagBits OldRenderer::get_max_usable_sample_count() const
{
    const auto device_properties = physical_device.getProperties();

    const auto counts = device_properties.limits.framebufferColorSampleCounts & device_properties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }
    return vk::SampleCountFlagBits::e1;
}

void OldRenderer::transition_image_layout(
    const uint32_t image_index,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask
    ) const
{
    transition_image_layout(
        command_buffers[current_frame],
        swap_chain_images[image_index],
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask
        );
}

void OldRenderer::transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::raii::Image& image,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    uint32_t mip_level
    ) const
{
    vk::PipelineStageFlags2 source_stage;
    vk::PipelineStageFlags2 destination_stage;
    vk::AccessFlags2 src_access_mask;
    vk::AccessFlags2 dst_access_mask;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        src_access_mask = {};
        dst_access_mask = vk::AccessFlagBits2::eTransferWrite;

        source_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits2::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        src_access_mask = vk::AccessFlagBits2::eTransferWrite;
        dst_access_mask = vk::AccessFlagBits2::eShaderRead;

        source_stage = vk::PipelineStageFlagBits2::eTransfer;
        destination_stage = vk::PipelineStageFlagBits2::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    transition_image_layout(
        command_buffer,
        image,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        source_stage,
        destination_stage,
        vk::ImageAspectFlagBits::eColor,
        mip_level
        );
}

void OldRenderer::transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask,
    const vk::ImageAspectFlags aspect_mask,
    const uint32_t mip_level
    ) const
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = mip_level,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    const vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    command_buffer.pipelineBarrier2(dependency_info);
}

}
