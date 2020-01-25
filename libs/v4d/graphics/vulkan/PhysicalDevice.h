/*
 * Vulkan Physical Device abstraction
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 */
#pragma once
#include "../../common.h"

namespace v4d::graphics::vulkan {

	class PhysicalDevice {
	private:
		xvk::Interface::InstanceInterface* vulkanInstance;
		VkPhysicalDevice handle;

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		
		std::vector<VkQueueFamilyProperties>* queueFamilies = nullptr;
		std::vector<VkExtensionProperties>* supportedExtensions = nullptr;

	public:
		PhysicalDevice(xvk::Interface::InstanceInterface* vulkanInstance, VkPhysicalDevice handle);
		~PhysicalDevice();

		// family queue index
		int GetQueueFamilyIndexFromFlags(VkDeviceQueueCreateFlags flags, uint minQueuesCount = 1, VkSurfaceKHR surface = nullptr);
		std::vector<int> GetQueueFamilyIndicesFromFlags(VkDeviceQueueCreateFlags flags, uint minQueuesCount = 1, VkSurfaceKHR surface = nullptr);
		bool QueueFamiliesContainsFlags(VkDeviceQueueCreateFlags flags, uint minQueuesCount = 1, VkSurfaceKHR surface = nullptr);

		bool SupportsExtension(std::string ext);

		VkPhysicalDeviceProperties GetProperties() const;
		VkPhysicalDeviceFeatures GetFeatures() const;
		VkPhysicalDevice GetHandle() const;
		xvk::Interface::InstanceInterface* GetVulkanInstance() const;
		std::string GetDescription() const;

		// create a logical device
		VkResult CreateDevice (const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);

		VkResult GetPhysicalDeviceSurfaceCapabilitiesKHR (VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities);
		VkResult GetPhysicalDeviceSurfaceFormatsKHR (VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats);
		VkResult GetPhysicalDeviceSurfacePresentModesKHR (VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes);
		void GetPhysicalDeviceFormatProperties (VkFormat format, VkFormatProperties* pFormatProperties);

		uint FindMemoryType(uint typeFilter, VkMemoryPropertyFlags properties);
		VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkSampleCountFlagBits GetMaxUsableSampleCount();

	};
}
