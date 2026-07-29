#define VK_USE_PLATFORM_WIN32_KHR
#include "Renderer/Lifetime/Context.hpp"

#include "Platform/Log.hpp"

#include <vulkan/vulkan_win32.h>

#include <array>
#include <vector>

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
);

namespace
{
const char* vulkan_get_severity_tag(VkDebugUtilsMessageSeverityFlagBitsEXT tag);
const char* vulkan_get_message_type_tag(VkDebugUtilsMessageTypeFlagsEXT tag);
}  // namespace

blk::Context
blk::create_context(HWND window)
{
	// ============================================================================
	// Create instance.
	// ============================================================================

	BLK_DEBUG("Creating instance...\n");

	VkApplicationInfo application_info = {};
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "Blink Game";
	application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	application_info.pEngineName = "Blink";
	application_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	application_info.apiVersion = VK_API_VERSION_1_3;

	constexpr std::array<const char*, 1> validation_layers = {"VK_LAYER_KHRONOS_validation"};
	constexpr std::array<const char*, 3> extensions =
		{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &application_info;
	instance_create_info.enabledLayerCount = validation_layers.size();
	instance_create_info.ppEnabledLayerNames = validation_layers.data();
	instance_create_info.enabledExtensionCount = extensions.size();
	instance_create_info.ppEnabledExtensionNames = extensions.data();

	VkInstance instance;

	if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create instance\n");
	}

	auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
	);

	if (!vkCreateDebugUtilsMessengerEXT)
	{
		BLK_FATAL("Failed to load vkCreateDebugUtilsMessengerEXT\n");
	}

	VkDebugUtilsMessengerEXT messenger;

	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {};
	messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
	messenger_create_info.pfnUserCallback = &vulkan_debug_callback;

	if (vkCreateDebugUtilsMessengerEXT(instance, &messenger_create_info, nullptr, &messenger) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create debug messenger\n");
	}

	// ============================================================================
	// Create surface.
	// ============================================================================

	BLK_DEBUG("Creating surface...\n");

	// FIXME: This is leaking outside of Platform/
	VkWin32SurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hinstance = GetModuleHandleW(nullptr);
	create_info.hwnd = window;

	VkSurfaceKHR surface;

	if (vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create Win32 surface\n");
	}

	// ============================================================================
	// Create device.
	// ============================================================================

	BLK_DEBUG("Selecting physical device...\n");

	uint32_t physical_device_count = 0;

	if (vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to get physical device count\n");
	}

	BLK_DEBUG("Got %u physical device(s)\n", physical_device_count);
	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);

	if (vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to enumerate physical devices\n");
	}

	BLK_DEBUG("Selecting best physical device...\n");
	bool found_physical_device = false;
	VkPhysicalDevice selected_physical_device;
	uint32_t graphics_queue_family_index = ~0;

	for (const auto& physical_device : physical_devices)
	{
		graphics_queue_family_index = ~0u;

		BLK_DEBUG("Getting physical device properties...\n");
		VkPhysicalDeviceProperties2 physical_device_properties = {};
		physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties);

		BLK_DEBUG("Getting physical device features...\n");
		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT physical_device_extended_dynamic_state_features = {};
		physical_device_extended_dynamic_state_features.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

		VkPhysicalDeviceVulkan13Features physical_device_vulkan_13_features = {};
		physical_device_vulkan_13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		physical_device_vulkan_13_features.pNext = &physical_device_extended_dynamic_state_features;

		VkPhysicalDeviceVulkan11Features physical_device_vulkan_11_features = {};
		physical_device_vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		physical_device_vulkan_11_features.pNext = &physical_device_vulkan_13_features;

		VkPhysicalDeviceFeatures2 physical_device_features = {};
		physical_device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physical_device_features.pNext = &physical_device_vulkan_11_features;
		vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features);

		BLK_DEBUG("Checking physical device: %s\n", physical_device_properties.properties.deviceName);

		if (physical_device_properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			BLK_DEBUG("Physical device is not a discrete GPU\n");
			continue;
		}

		if (!physical_device_features.features.geometryShader)
		{
			BLK_DEBUG("Physical device does not support geometry shaders\n");
			continue;
		}

		if (!physical_device_features.features.samplerAnisotropy)
		{
			BLK_DEBUG("Physical device does not support anisotropy filter\n");
			continue;
		}

		if (physical_device_properties.properties.apiVersion < VK_API_VERSION_1_3)
		{
			BLK_DEBUG("Physical device does not support Vulkan 1.3 or higher\n");
			continue;
		}

		if (!physical_device_vulkan_11_features.shaderDrawParameters)
		{
			BLK_DEBUG("Physical device does not support shader draw parameters\n");
			continue;
		}

		if (!physical_device_vulkan_13_features.dynamicRendering)
		{
			BLK_DEBUG("Physical device does not support dynamic rendering");
			continue;
		}

		if (!physical_device_vulkan_13_features.synchronization2)
		{
			BLK_DEBUG("Physical device does not support synchronization 2\n");
			continue;
		}

		if (!physical_device_extended_dynamic_state_features.extendedDynamicState)
		{
			BLK_DEBUG("Physical device does not support extended dynamic state\n");
			continue;
		}

		BLK_DEBUG("Getting queue family properties...\n");
		uint32_t queque_family_properties_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queque_family_properties_count, nullptr);

		BLK_DEBUG("Got %u queue(s) family properties\n", queque_family_properties_count);
		std::vector<VkQueueFamilyProperties2> queue_families_properties(queque_family_properties_count);

		for (auto& queue_family_properties : queue_families_properties)
		{
			queue_family_properties.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		vkGetPhysicalDeviceQueueFamilyProperties2(
			physical_device,
			&queque_family_properties_count,
			queue_families_properties.data()
		);

		BLK_DEBUG("Checking surface support for physical device...\n");

		for (uint32_t queue_family_properties_index = 0;
			 queue_family_properties_index < queue_families_properties.size();
			 queue_family_properties_index++)
		{
			VkBool32 supports_surface = VK_FALSE;

			if (vkGetPhysicalDeviceSurfaceSupportKHR(
					physical_device,
					queue_family_properties_index,
					surface,
					&supports_surface
				) != VK_SUCCESS)
			{
				BLK_FATAL("Failed to get physical device surface support\n");
			}

			VkQueueFamilyProperties2 queue_family_properties = queue_families_properties[queue_family_properties_index];
			const auto supports_graphics =
				static_cast<bool>(queue_family_properties.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT);

			if (supports_graphics && supports_surface)
			{
				graphics_queue_family_index = queue_family_properties_index;
				break;
			}
		}

		if (graphics_queue_family_index == ~0)
		{
			BLK_DEBUG("Failed to find suitable queue\n");
			continue;
		}

		BLK_DEBUG("Getting physical device extension properties...\n");
		uint32_t physical_device_extension_properties_count = 0;

		if (vkEnumerateDeviceExtensionProperties(
				physical_device,
				nullptr,
				&physical_device_extension_properties_count,
				nullptr
			) != VK_SUCCESS)
		{
			BLK_FATAL("Failed to get physical device extension properties count\n");
		}

		BLK_DEBUG("Got %u physical device extension properties\n", physical_device_extension_properties_count);
		std::vector<VkExtensionProperties> physical_device_extension_properties(
			physical_device_extension_properties_count
		);

		if (vkEnumerateDeviceExtensionProperties(
				physical_device,
				nullptr,
				&physical_device_extension_properties_count,
				physical_device_extension_properties.data()
			) != VK_SUCCESS)
		{
			BLK_FATAL("Failed to get physical device extension properties\n");
		}

		BLK_DEBUG("Checking physical device extensions supports...\n");
		constexpr std::array<const char*, 1> required_physical_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		bool supports_extension = false;

		for (const auto& required_extension : required_physical_device_extensions)
		{
			supports_extension = false;

			for (const auto& extension_properties : physical_device_extension_properties)
			{
				if (strcmp(required_extension, extension_properties.extensionName) == 0)
				{
					supports_extension = true;
					break;
				}
			}

			if (!supports_extension)
			{
				BLK_DEBUG("Required physical device extension not supported: %s\n", required_extension);
				break;
			}
		}

		if (!supports_extension)
		{
			continue;
		}

		BLK_DEBUG("Selected device: %s\n", physical_device_properties.properties.deviceName);
		selected_physical_device = physical_device;
		found_physical_device = true;
	}

	if (!found_physical_device)
	{
		BLK_FATAL("Failed to found physical device\n");
	}

	BLK_DEBUG("Creating logical device...\n");

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT device_required_extended_dynamic_state_features = {};
	device_required_extended_dynamic_state_features.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	device_required_extended_dynamic_state_features.extendedDynamicState = VK_TRUE;

	VkPhysicalDeviceVulkan13Features device_required_vulkan_13_features = {};
	device_required_vulkan_13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	device_required_vulkan_13_features.pNext = &device_required_extended_dynamic_state_features;
	device_required_vulkan_13_features.dynamicRendering = VK_TRUE;
	device_required_vulkan_13_features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceVulkan11Features device_required_vulkan_11_features = {};
	device_required_vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	device_required_vulkan_11_features.pNext = &device_required_vulkan_13_features;
	device_required_vulkan_11_features.shaderDrawParameters = VK_TRUE;

	VkPhysicalDeviceFeatures2 device_required_features = {};
	device_required_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_required_features.pNext = &device_required_vulkan_11_features;
	device_required_features.features.samplerAnisotropy = VK_TRUE;

	VkDeviceQueueCreateInfo device_queue_create_info = {};
	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.queueFamilyIndex = graphics_queue_family_index;
	device_queue_create_info.queueCount = 1;
	constexpr float queue_priority = 0.5f;
	device_queue_create_info.pQueuePriorities = &queue_priority;

	constexpr std::array<const char*, 1> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext = &device_required_features;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &device_queue_create_info;
	device_create_info.enabledExtensionCount = device_extensions.size();
	device_create_info.ppEnabledExtensionNames = device_extensions.data();

	VkDevice logical_device;

	if (vkCreateDevice(selected_physical_device, &device_create_info, nullptr, &logical_device) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create logical device\n");
	}

	BLK_DEBUG("Getting graphics queue handle...\n");
	VkQueue graphics_queue;
	vkGetDeviceQueue(logical_device, graphics_queue_family_index, 0, &graphics_queue);

	// ============================================================================
	// Create command pools.
	// ============================================================================

	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	command_pool_info.queueFamilyIndex = graphics_queue_family_index;

	VkCommandPool transient_command_pool;

	if (vkCreateCommandPool(logical_device, &command_pool_info, nullptr, &transient_command_pool) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create command pool\n");
	}

	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool frame_command_pool;

	if (vkCreateCommandPool(logical_device, &command_pool_info, nullptr, &frame_command_pool) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create command pool\n");
	}

	// ============================================================================
	// Create the shared texture sampler.
	// ============================================================================

	VkPhysicalDeviceProperties physical_device_properties = {};
	vkGetPhysicalDeviceProperties(selected_physical_device, &physical_device_properties);

	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter = VK_FILTER_LINEAR;
	sampler_create_info.minFilter = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.anisotropyEnable = VK_TRUE;
	sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
	sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;

	VkSampler sampler;

	if (vkCreateSampler(logical_device, &sampler_create_info, nullptr, &sampler) != VK_SUCCESS)
	{
		BLK_FATAL("Failed to create sampler\n");
	}

	return Context{
		.api_version = application_info.apiVersion,
		.instance = instance,
		.surface = surface,
		.physical_device = selected_physical_device,
		.logical_device = logical_device,
		.graphics_queue = graphics_queue,
		.graphics_queue_family_index = graphics_queue_family_index,
		.frame_command_pool = frame_command_pool,
		.transient_command_pool = transient_command_pool,
		.sampler = sampler,
	};
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
)
{
	(void)user_data;

	blk::log_msg(
		"VULKAN",
		nullptr,
		0,
		"[ %s ] [ %s ] %s\n",
		vulkan_get_severity_tag(message_severity),
		vulkan_get_message_type_tag(message_type),
		callback_data->pMessage
	);

	return VK_FALSE;
}

namespace
{
const char*

vulkan_get_severity_tag(VkDebugUtilsMessageSeverityFlagBitsEXT tag)
{
	switch (tag)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "VERBOSE";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "INFO";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "WARNING";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

const char*
vulkan_get_message_type_tag(VkDebugUtilsMessageTypeFlagsEXT tag)
{
	switch (tag)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		return "GENERAL";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		return "VALIDATION";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		return "PERFORMANCE";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
		return "ADDRESS_BINDING";
	default:
		// TODO: This is not correct. Multiple message type flags could be set, so this switch will return `UNKNOWN`
		//   incorrectly.
		return "UNKNOWN";
	}
}
}  // namespace
