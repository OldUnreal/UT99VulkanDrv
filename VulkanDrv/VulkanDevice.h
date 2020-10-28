#pragma once

class VulkanSwapChain;
class VulkanSemaphore;
class VulkanFence;

class VulkanPhysicalDevice
{
public:
	VkPhysicalDevice device = VK_NULL_HANDLE;

	std::vector<VkExtensionProperties> extensions;
	std::vector<VkQueueFamilyProperties> queueFamilies;
	VkPhysicalDeviceProperties properties = {};
	VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties = {};
	VkPhysicalDeviceFeatures features = {};
	VkPhysicalDeviceMemoryProperties memoryProperties = {};
};

class VulkanCompatibleDevice
{
public:
	VulkanPhysicalDevice *device = nullptr;
	int graphicsFamily = -1;
	int presentFamily = -1;
};

class VulkanDevice
{
public:
	VulkanDevice(HWND window);
	~VulkanDevice();

	void setDebugObjectName(const char *name, uint64_t handle, VkObjectType type)
	{
		if (!debugLayerActive) return;

		VkDebugUtilsObjectNameInfoEXT info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		info.objectHandle = handle;
		info.objectType = type;
		info.pObjectName = name;
		vkSetDebugUtilsObjectNameEXT(device, &info);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	HWND window;

	// Instance setup
	std::vector<VkLayerProperties> availableLayers;
	std::vector<VkExtensionProperties> extensions;
	std::vector<const char *> enabledExtensions;
	std::vector<const char *> optionalExtensions = { VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME };
	std::vector<const char*> enabledValidationLayers;

	// Device setup
	VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
	std::vector<const char *> enabledDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	std::vector<const char *> optionalDeviceExtensions = {
		VK_EXT_HDR_METADATA_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_NV_RAY_TRACING_EXTENSION_NAME
	};
	VulkanPhysicalDevice physicalDevice;
	bool debugLayerActive = false;

	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VmaAllocator allocator = VK_NULL_HANDLE;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	int graphicsFamily = -1;
	int presentFamily = -1;

private:
	void createInstance();
	void createSurface();
	void selectPhysicalDevice();
	void selectFeatures();
	void createDevice();
	void createAllocator();
	void releaseResources();

	bool supportsDeviceExtension(const char *ext) const;

	static bool checkRequiredFeatures(const VkPhysicalDeviceFeatures &f);

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	static void initVolk();
	static std::vector<VkLayerProperties> getAvailableLayers();
	static std::vector<VkExtensionProperties> getExtensions();
	static std::vector<const char *> getPlatformExtensions();
	static std::vector<VulkanPhysicalDevice> getPhysicalDevices(VkInstance instance);
};
