#include "../../common.h"

using namespace v4d::graphics::vulkan;

PhysicalDevice::PhysicalDevice(xvk::Interface::InstanceInterface* vulkanInstance, VkPhysicalDevice handle) : vulkanInstance(vulkanInstance), handle(handle) {
	// Properties
	vulkanInstance->GetPhysicalDeviceProperties(handle, &deviceProperties);
	LOG("DETECTED PhysicalDevice: " << deviceProperties.deviceName);
	// Features
	vulkanInstance->GetPhysicalDeviceFeatures(handle, &deviceFeatures);
	// Queue Families
	uint queueFamilyCount = 0;
	vulkanInstance->GetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, nullptr);
	queueFamilies = new std::vector<VkQueueFamilyProperties>(queueFamilyCount);
	vulkanInstance->GetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, queueFamilies->data());
	// Supported Extensions
	uint supportedExtensionsCount = 0;
	vulkanInstance->EnumerateDeviceExtensionProperties(handle, nullptr, &supportedExtensionsCount, nullptr);
	supportedExtensions = new std::vector<VkExtensionProperties>(supportedExtensionsCount);
	vulkanInstance->EnumerateDeviceExtensionProperties(handle, nullptr, &supportedExtensionsCount, supportedExtensions->data());
}

PhysicalDevice::~PhysicalDevice() {
	delete queueFamilies;
	delete supportedExtensions;
}

int PhysicalDevice::GetQueueFamilyIndexFromFlags(VkDeviceQueueCreateFlags flags, uint minQueuesCount, VkSurfaceKHR surface) {
	int i = 0;
	for (const auto& queueFamily : *queueFamilies) {
		if (queueFamily.queueCount >= minQueuesCount && queueFamily.queueFlags & flags) {
			if (surface == nullptr) return i;
			VkBool32 presentationSupport;
			vulkanInstance->GetPhysicalDeviceSurfaceSupportKHR(handle, i, surface, &presentationSupport);
			if (presentationSupport) return i;
		}
		i++;
	}
	return -1;
}

std::vector<int> PhysicalDevice::GetQueueFamilyIndicesFromFlags(VkDeviceQueueCreateFlags flags, uint minQueuesCount, VkSurfaceKHR surface) {
	std::vector<int> indices {};
	int i = 0;
	for (const auto& queueFamily : *queueFamilies) {
		if (queueFamily.queueCount >= minQueuesCount && queueFamily.queueFlags & flags) {
			if (surface == nullptr) {
				indices.push_back(i);
			} else {
				VkBool32 presentationSupport;
				vulkanInstance->GetPhysicalDeviceSurfaceSupportKHR(handle, i, surface, &presentationSupport);
				if (presentationSupport) indices.push_back(i);
			}
		}
		i++;
	}
	return indices;
}

bool PhysicalDevice::QueueFamiliesContainsFlags(VkDeviceQueueCreateFlags flags, uint minQueuesCount, VkSurfaceKHR surface) {
	for (const auto& queueFamily : *queueFamilies) {
		if (queueFamily.queueCount >= minQueuesCount && queueFamily.queueFlags & flags) {
			if (surface == nullptr) return true;
			VkBool32 presentationSupport;
			vulkanInstance->GetPhysicalDeviceSurfaceSupportKHR(handle, 0, surface, &presentationSupport);
			if (presentationSupport) return true;
		}
	}
	return false;
}

bool PhysicalDevice::SupportsExtension(std::string ext) {
	for (const auto& extension : *supportedExtensions) {
		if (extension.extensionName == ext) {
			return true;
		}
	}
	return false;
}

VkPhysicalDeviceProperties PhysicalDevice::GetProperties() const {
	return deviceProperties;
}

VkPhysicalDeviceFeatures PhysicalDevice::GetFeatures() const {
	return deviceFeatures;
}

VkPhysicalDevice PhysicalDevice::GetHandle() const {
	return handle;
}

xvk::Interface::InstanceInterface* PhysicalDevice::GetVulkanInstance() const {
	return vulkanInstance;
}

VkResult PhysicalDevice::CreateDevice (const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
	return vulkanInstance->CreateDevice(handle, pCreateInfo, pAllocator, pDevice);
}

std::string PhysicalDevice::GetDescription() const {
	return GetProperties().deviceName;
}

VkResult PhysicalDevice::GetPhysicalDeviceSurfaceCapabilitiesKHR (VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) {
	return vulkanInstance->GetPhysicalDeviceSurfaceCapabilitiesKHR(handle, surface, pSurfaceCapabilities);
}

VkResult PhysicalDevice::GetPhysicalDeviceSurfaceFormatsKHR (VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) {
	return vulkanInstance->GetPhysicalDeviceSurfaceFormatsKHR(handle, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult PhysicalDevice::GetPhysicalDeviceSurfacePresentModesKHR (VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) {
	return vulkanInstance->GetPhysicalDeviceSurfacePresentModesKHR(handle, surface, pPresentModeCount, pPresentModes);
}

void PhysicalDevice::GetPhysicalDeviceFormatProperties (VkFormat format, VkFormatProperties* pFormatProperties) {
	vulkanInstance->GetPhysicalDeviceFormatProperties(handle, format, pFormatProperties);
}

uint PhysicalDevice::FindMemoryType(uint typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vulkanInstance->GetPhysicalDeviceMemoryProperties(handle, &memProperties);
	for (uint i = 0; i < memProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type");
}

VkFormat PhysicalDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vulkanInstance->GetPhysicalDeviceFormatProperties(handle, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("Failed to find supported format");
}

VkSampleCountFlagBits PhysicalDevice::GetMaxUsableSampleCount() {
	VkSampleCountFlags counts = std::min(GetProperties().limits.framebufferColorSampleCounts, GetProperties().limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
	return VK_SAMPLE_COUNT_1_BIT;
}
