//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "swapchain.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
vk::Extent2D choose_extent(
	vk::Extent2D request_extent,
	const vk::Extent2D& min_image_extent,
	const vk::Extent2D& max_image_extent,
	const vk::Extent2D& current_extent
)
{
	if (current_extent.width == 0xFFFFFFFF)
		return request_extent;

	if (request_extent.width < 1 || request_extent.height < 1)
	{
		LOG_CORE_WARN_TAG(
			"Vulkan",
			"(Swapchain) Image extent ({}, {}) not supported. Selecting ({}, {}).",
			request_extent.width,
			request_extent.height,
			current_extent.width,
			current_extent.height
		);
		return current_extent;
	}

	request_extent.width = glm::clamp(request_extent.width, min_image_extent.width, max_image_extent.width);
	request_extent.height = glm::clamp(request_extent.height, min_image_extent.height, max_image_extent.height);

	return request_extent;
}

vk::PresentModeKHR choose_present_mode(
	const vk::PresentModeKHR request_present_mode,
	const std::vector<vk::PresentModeKHR>& available_present_modes,
	const std::vector<vk::PresentModeKHR>& present_mode_priority_list
)
{
	// Try to find the requested present mode in the available present modes
	auto const present_mode_it = std::ranges::find(available_present_modes, request_present_mode);
	if (present_mode_it == available_present_modes.end())
	{
		// If the requested present mode isn't found, then try to find a mode from the priority list
		auto const chosen_present_mode_it =
			std::ranges::find_if(
				present_mode_priority_list,
				[&available_present_modes](vk::PresentModeKHR present_mode)
				{
					return std::ranges::find(available_present_modes, present_mode) != available_present_modes.end();
				}
			);

		// If nothing found, always default to FIFO
		const vk::PresentModeKHR chosen_present_mode = (chosen_present_mode_it != present_mode_priority_list.end())
			                                               ? *chosen_present_mode_it
			                                               : vk::PresentModeKHR::eFifo;

		LOG_CORE_WARN_TAG(
			"Vulkan",
			"(Swapchain) Present mode '{}' not supported. Selecting '{}'.",
			vk::to_string(request_present_mode),
			vk::to_string(chosen_present_mode)
		);
		return chosen_present_mode;
	}
	LOG_CORE_INFO_TAG("Vulkan", "(Swapchain) Present mode selected: {}", to_string(request_present_mode));
	return request_present_mode;
}

vk::SurfaceFormatKHR choose_surface_format(
	const vk::SurfaceFormatKHR requested_surface_format,
	const std::vector<vk::SurfaceFormatKHR>& available_surface_formats,
	const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list
)
{
	// Try to find the requested surface format in the available surface formats
	auto const surface_format_it = std::ranges::find(available_surface_formats, requested_surface_format);

	// If the requested surface format isn't found, then try to request a format from the priority list
	if (surface_format_it == available_surface_formats.end())
	{
		auto const chosen_surface_format_it =
			std::ranges::find_if(
				surface_format_priority_list,
				[&available_surface_formats](vk::SurfaceFormatKHR surface_format)
				{
					return std::ranges::find(available_surface_formats, surface_format) != available_surface_formats.
						end();
				}
			);

		// If nothing found, default to the first available format
		const vk::SurfaceFormatKHR& chosen_surface_format = (chosen_surface_format_it != surface_format_priority_list.end())
			                                                    ? *chosen_surface_format_it
			                                                    : available_surface_formats[0];

		LOG_CORE_WARN_TAG(
			"Vulkan",
			"(Swapchain) Surface format ({}) not supported. Selecting ({}).",
			vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace),
			vk::to_string(chosen_surface_format.format) + ", " + vk::to_string(chosen_surface_format.colorSpace)
		);
		return chosen_surface_format;
	}

	LOG_CORE_INFO_TAG(
		"Vulkan",
		"(Swapchain) Surface format selected: {}",
		vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace)
	);
	return requested_surface_format;
}

vk::SurfaceTransformFlagBitsKHR choose_transform(
	const vk::SurfaceTransformFlagBitsKHR request_transform,
	const vk::SurfaceTransformFlagsKHR supported_transform,
	const vk::SurfaceTransformFlagBitsKHR current_transform
)
{
	if (request_transform & supported_transform)
	{
		return request_transform;
	}

	LOG_CORE_WARN_TAG(
		"Vulkan",
		"(Swapchain) Surface transform '{}' not supported. Selecting '{}'.",
		vk::to_string(request_transform),
		vk::to_string(current_transform)
	);
	return current_transform;
}

vk::CompositeAlphaFlagBitsKHR choose_composite_alpha(
	const vk::CompositeAlphaFlagBitsKHR request_composite_alpha,
	vk::CompositeAlphaFlagsKHR supported_composite_alpha
)
{
	if (request_composite_alpha & supported_composite_alpha)
	{
		return request_composite_alpha;
	}

	static const std::vector<vk::CompositeAlphaFlagBitsKHR> composite_alpha_priority_list = {
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
		vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
		vk::CompositeAlphaFlagBitsKHR::eInherit
	};

	auto const chosen_composite_alpha_it =
		std::find_if(
			composite_alpha_priority_list.begin(),
			composite_alpha_priority_list.end(),
			[&supported_composite_alpha](vk::CompositeAlphaFlagBitsKHR composite_alpha) { return composite_alpha & supported_composite_alpha; }
		);
	if (chosen_composite_alpha_it == composite_alpha_priority_list.end())
	{
		throw std::runtime_error("No compatible composite alpha found.");
	}
	LOG_CORE_WARN_TAG(
		"Vulkan",
		"(Swapchain) Composite alpha '{}' not supported. Selecting '{}.",
		vk::to_string(request_composite_alpha),
		vk::to_string(*chosen_composite_alpha_it)
	);
	return *chosen_composite_alpha_it;
}

bool validate_format_feature(const vk::ImageUsageFlagBits image_usage, const vk::FormatFeatureFlags supported_features)
{
	return (image_usage != vk::ImageUsageFlagBits::eStorage) || (supported_features & vk::FormatFeatureFlagBits::eStorageImage);
}

std::set<vk::ImageUsageFlagBits> choose_image_usage(
	const std::set<vk::ImageUsageFlagBits>& requested_image_usage_flags,
	vk::ImageUsageFlags supported_image_usage,
	vk::FormatFeatureFlags supported_features
)
{
	std::set<vk::ImageUsageFlagBits> validated_image_usage_flags;
	for (auto flag : requested_image_usage_flags)
	{
		if ((flag & supported_image_usage) && validate_format_feature(flag, supported_features))
		{
			validated_image_usage_flags.insert(flag);
		}
		else
		{
			LOG_CORE_WARN_TAG("Vulkan", "(Swapchain) Image usage ({}) requested but not supported.", vk::to_string(flag));
		}
	}

	if (validated_image_usage_flags.empty())
	{
		// Pick the first format from list of defaults, if supported
		static const std::vector<vk::ImageUsageFlagBits> image_usage_priority_list = {
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::ImageUsageFlagBits::eStorage,
			vk::ImageUsageFlagBits::eSampled,
			vk::ImageUsageFlagBits::eTransferDst
		};

		auto const priority_list_it =
			std::ranges::find_if(
				image_usage_priority_list,
				[&supported_image_usage, &supported_features](auto const image_usage)
				{
					return ((image_usage & supported_image_usage) && validate_format_feature(image_usage, supported_features));
				}
			);
		if (priority_list_it != image_usage_priority_list.end())
		{
			validated_image_usage_flags.insert(*priority_list_it);
		}
	}

	if (validated_image_usage_flags.empty())
	{
		throw std::runtime_error("No compatible image usage found.");
	}
	else
	{
		// Log image usage flags used
		std::string usage_list;
		for (vk::ImageUsageFlagBits image_usage : validated_image_usage_flags)
		{
			usage_list += to_string(image_usage) + " ";
		}
		LOG_CORE_INFO_TAG("Vulkan", "(Swapchain) Image usage flags: {}", usage_list);
	}

	return validated_image_usage_flags;
}

vk::ImageUsageFlags composite_image_flags(const std::set<vk::ImageUsageFlagBits>& image_usage_flags)
{
	vk::ImageUsageFlags image_usage;
	for (const auto flag : image_usage_flags)
	{
		image_usage |= flag;
	}
	return image_usage;
}


Swapchain::Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent)
	: Swapchain(
		old_swapchain,
		old_swapchain.get_device(),
		old_swapchain.surface,
		old_swapchain.properties.present_mode,
		old_swapchain.present_mode_priority_list,
		old_swapchain.surface_format_priority_list,
		extent,
		old_swapchain.properties.image_count,
		old_swapchain.properties.pre_transform,
		old_swapchain.image_usage_flags,
		old_swapchain.requested_compression,
		old_swapchain.requested_compression_fixed_rate
	)
{}

Swapchain::Swapchain(Swapchain& old_swapchain, const uint32_t image_count)
	: Swapchain(
		old_swapchain,
		old_swapchain.get_device(),
		old_swapchain.surface,
		old_swapchain.properties.present_mode,
		old_swapchain.present_mode_priority_list,
		old_swapchain.surface_format_priority_list,
		old_swapchain.properties.extent,
		image_count,
		old_swapchain.properties.pre_transform,
		old_swapchain.image_usage_flags,
		old_swapchain.requested_compression,
		old_swapchain.requested_compression_fixed_rate
	)
{}

Swapchain::Swapchain(Swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags)
	: Swapchain(
		old_swapchain,
		old_swapchain.get_device(),
		old_swapchain.surface,
		old_swapchain.properties.present_mode,
		old_swapchain.present_mode_priority_list,
		old_swapchain.surface_format_priority_list,
		old_swapchain.properties.extent,
		old_swapchain.properties.image_count,
		old_swapchain.properties.pre_transform,
		image_usage_flags,
		old_swapchain.requested_compression,
		old_swapchain.requested_compression_fixed_rate
	)
{}

Swapchain::Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform)
	: Swapchain(
		old_swapchain,
		old_swapchain.get_device(),
		old_swapchain.surface,
		old_swapchain.properties.present_mode,
		old_swapchain.present_mode_priority_list,
		old_swapchain.surface_format_priority_list,
		extent,
		old_swapchain.properties.image_count,
		transform,
		old_swapchain.image_usage_flags,
		old_swapchain.requested_compression,
		old_swapchain.requested_compression_fixed_rate
	)
{}

Swapchain::Swapchain(
	Swapchain& old_swapchain,
	const vk::ImageCompressionFlagsEXT requested_compression,
	const vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate
) : Swapchain(
	old_swapchain,
	old_swapchain.get_device(),
	old_swapchain.surface,
	old_swapchain.properties.present_mode,
	old_swapchain.present_mode_priority_list,
	old_swapchain.surface_format_priority_list,
	old_swapchain.properties.extent,
	old_swapchain.properties.image_count,
	old_swapchain.properties.pre_transform,
	old_swapchain.image_usage_flags,
	requested_compression,
	requested_compression_fixed_rate
)
{}

Swapchain::Swapchain(
	Device& device,
	vk::SurfaceKHR surface,
	const vk::PresentModeKHR present_mode,
	const std::vector<vk::PresentModeKHR>& present_mode_priority_list,
	const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list,
	const vk::Extent2D& extent,
	const uint32_t image_count,
	const vk::SurfaceTransformFlagBitsKHR transform,
	const std::set<vk::ImageUsageFlagBits>& image_usage_flags,
	const vk::ImageCompressionFlagsEXT requested_compression,
	const vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate
): Swapchain(
	*this,
	device,
	surface,
	present_mode,
	present_mode_priority_list,
	surface_format_priority_list,
	extent,
	image_count,
	transform,
	image_usage_flags
)
{}

Swapchain::Swapchain(
	Swapchain& old_swapchain,
	Device& device,
	vk::SurfaceKHR surface,
	const vk::PresentModeKHR present_mode,
	const std::vector<vk::PresentModeKHR>& present_mode_priority_list,
	const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list,
	const vk::Extent2D& extent,
	const uint32_t image_count,
	const vk::SurfaceTransformFlagBitsKHR transform,
	const std::set<vk::ImageUsageFlagBits>& image_usage_flags,
	const vk::ImageCompressionFlagsEXT requested_compression,
	const vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate
): VulkanResource(nullptr, &device),
   surface(surface),
   requested_compression{requested_compression},
   requested_compression_fixed_rate{requested_compression_fixed_rate}
{
	this->present_mode_priority_list = present_mode_priority_list;
	this->surface_format_priority_list = surface_format_priority_list;

	const auto surface_formats = device.get_gpu().get_handle().getSurfaceFormatsKHR(surface);
	LOG_CORE_INFO_TAG("Vulkan", "Surface supports the following surface formats:");
	for (const auto& surface_format : surface_formats)
	{
		LOG_CORE_INFO_TAG("Vulkan", "  \t{}", vk::to_string(surface_format.format) + ", " + vk::to_string(surface_format.colorSpace));
	}

	const auto present_modes = device.get_gpu().get_handle().getSurfacePresentModesKHR(surface);
	LOG_CORE_INFO_TAG("Vulkan", "Surface supports the following present modes:");
	for (auto& pr : present_modes)
	{
		LOG_CORE_INFO_TAG("Vulkan", "  \t{}", to_string(pr));
	}

	// Choose the best properties based on surface capabilities
	const auto surface_capabilities = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);

	properties.old_swapchain = old_swapchain.get_handle();
	properties.image_count = glm::clamp(
		image_count,
		surface_capabilities.minImageCount,
		surface_capabilities.maxImageCount ? surface_capabilities.maxImageCount : (std::numeric_limits<uint32_t>::max)()
	);
	properties.extent = choose_extent(
		extent,
		surface_capabilities.minImageExtent,
		surface_capabilities.maxImageExtent,
		surface_capabilities.currentExtent
	);
	properties.surface_format = choose_surface_format(properties.surface_format, surface_formats, surface_format_priority_list);
	properties.array_layers = 1;

	const auto format_properties = device.get_gpu().get_handle().getFormatProperties(properties.surface_format.format);
	this->image_usage_flags = choose_image_usage(
		image_usage_flags,
		surface_capabilities.supportedUsageFlags,
		format_properties.optimalTilingFeatures
	);

	properties.image_usage = composite_image_flags(this->image_usage_flags);
	properties.pre_transform = choose_transform(transform, surface_capabilities.supportedTransforms, surface_capabilities.currentTransform);
	properties.composite_alpha = choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR::eInherit, surface_capabilities.supportedCompositeAlpha);
	properties.present_mode = choose_present_mode(present_mode, present_modes, present_mode_priority_list);

	vk::SwapchainCreateInfoKHR create_info(
		{},
		surface,
		properties.image_count,
		properties.surface_format.format,
		properties.surface_format.colorSpace,
		properties.extent,
		properties.array_layers,
		properties.image_usage,
		{},
		{},
		properties.pre_transform,
		properties.composite_alpha,
		properties.present_mode,
		{},
		properties.old_swapchain
	);

	auto fixed_rate_flags = requested_compression_fixed_rate;
	vk::ImageCompressionControlEXT compression_control{};
	compression_control.flags = requested_compression;
	if (device.is_enabled(VK_EXT_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_EXTENSION_NAME))
	{
		create_info.pNext = &compression_control;
		if (vk::ImageCompressionFlagBitsEXT::eFixedRateExplicit == requested_compression)
		{
			// Do not support compression for multi-planar formats
			compression_control.compressionControlPlaneCount = 1;
			compression_control.pFixedRateFlags = &fixed_rate_flags;
		}
		else if (vk::ImageCompressionFlagBitsEXT::eDisabled == requested_compression)
		{
			LOG_CORE_WARN_TAG("Vulkan", "(Swapchain) Disabling default (lossless) compression, which can negatively impact performance");
		}
	}
	else
	{
		if (vk::ImageCompressionFlagBitsEXT::eDefault != requested_compression)
		{
			LOG_CORE_WARN_TAG(
				"Vulkan",
				"(Swapchain) Compression cannot be controlled because VK_EXT_image_compression_control_swapchain is not enabled"
			);

			this->requested_compression = vk::ImageCompressionFlagBitsEXT::eDefault;
			this->requested_compression_fixed_rate = vk::ImageCompressionFixedRateFlagBitsEXT::eNone;
		}
	}

	set_handle(device.get_handle().createSwapchainKHR(create_info));
	images = device.get_handle().getSwapchainImagesKHR(get_handle());
}

Swapchain::Swapchain(Swapchain&& other) noexcept:
	VulkanResource(std::move(other)),
	surface{std::exchange(other.surface, nullptr)},
	images{std::exchange(other.images, {})},
	properties{std::exchange(other.properties, {})},
	present_mode_priority_list{std::exchange(other.present_mode_priority_list, {})},
	surface_format_priority_list{std::exchange(other.surface_format_priority_list, {})},
	image_usage_flags{std::move(other.image_usage_flags)}
{}

Swapchain::~Swapchain()
{
	if (has_handle())
		get_device_handle().destroySwapchainKHR(get_handle());
}

bool Swapchain::is_valid() const
{
	return !!get_handle();
}

std::pair<vk::Result, uint32_t> Swapchain::acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence) const
{
	vk::ResultValue<uint32_t> rv = get_device_handle().acquireNextImageKHR(
		get_handle(),
		(std::numeric_limits<uint64_t>::max)(),
		image_acquired_semaphore,
		fence
	);
	return std::make_pair(rv.result, rv.value);
}

const vk::Extent2D& Swapchain::get_extent() const
{
	return properties.extent;
}

vk::Format Swapchain::get_format() const
{
	return properties.surface_format.format;
}

vk::SurfaceFormatKHR Swapchain::get_surface_format() const
{
	return properties.surface_format;
}

const std::vector<vk::Image>& Swapchain::get_images() const
{
	return images;
}

vk::SurfaceTransformFlagBitsKHR Swapchain::get_transform() const
{
	return properties.pre_transform;
}

vk::SurfaceKHR Swapchain::get_surface() const
{
	return surface;
}

vk::ImageUsageFlags Swapchain::get_usage() const
{
	return properties.image_usage;
}

vk::PresentModeKHR Swapchain::get_present_mode() const
{
	return properties.present_mode;
}
} // portal
