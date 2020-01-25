/*
 * Vulkan SwapChain abstraction
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 */
#pragma once

#include "../../common.h"

namespace v4d::graphics::vulkan {
	
	struct SwapChain {
	private:
		Device* device;
		VkSurfaceKHR surface;
		VkSwapchainKHR handle;

	private:
		// Supported configurations
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

	public:
		// SwapChain images
		std::vector<VkImage> images {};
		
		// Selected configuration
		VkSurfaceFormatKHR format;
		VkPresentModeKHR presentMode;
		VkExtent2D extent;

		VkPipelineViewportStateCreateInfo viewportState {};
		VkViewport viewport {};
		VkRect2D scissor {};
		VkSwapchainCreateInfoKHR createInfo {};
		VkImageViewCreateInfo imageViewsCreateInfo {};
		std::vector<VkImageView> imageViews {};

		// Constructor
		SwapChain();
		SwapChain(Device* device, VkSurfaceKHR surface);
		SwapChain(
			Device* device, 
			VkSurfaceKHR surface, 
			VkExtent2D preferredExtent, 
			const std::vector<VkSurfaceFormatKHR> preferredFormats, 
			const std::vector<VkPresentModeKHR> preferredPresentModes
		);
		~SwapChain();

		VkSwapchainKHR GetHandle() const;

		void SetConfiguration(VkExtent2D preferredExtent, const std::vector<VkSurfaceFormatKHR> preferredFormats, const std::vector<VkPresentModeKHR>& preferredPresentModes);
		void AssignQueues(std::vector<uint32_t> queues);

		void Create(SwapChain* oldSwapChain = nullptr);
		void Destroy();

		void ResolveCapabilities(VkPhysicalDevice physicalDevice);
		void ResolveFormats(VkPhysicalDevice physicalDevice);
		void ResolvePresentModes(VkPhysicalDevice physicalDevice);

		VkExtent2D GetPreferredExtent(VkExtent2D preferredExtent);
		VkSurfaceFormatKHR GetPreferredSurfaceFormat(const std::vector<VkSurfaceFormatKHR> preferredFormats);
		VkPresentModeKHR GetPreferredPresentMode(const std::vector<VkPresentModeKHR>& preferredPresentModes);
	};
}
