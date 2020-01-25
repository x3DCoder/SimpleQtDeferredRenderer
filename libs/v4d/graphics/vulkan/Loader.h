/*
 * Vulkan Dynamic Loader
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 * 
 * This class extends from xvk's dynamic loader (xvk is another open-source library related to Vulkan4D)
 */
#pragma once
#include "../../common.h"

#define XVK_INTERFACE_RAW_FUNCTIONS_ACCESSIBILITY private
#include <xvk.hpp>

#define V4D_ENGINE_NAME "Vulkan4D"
#define V4D_ENGINE_VERSION VK_MAKE_VERSION(1, 0, 0)
#define VULKAN_API_VERSION VK_API_VERSION_1_1

namespace v4d::graphics::vulkan {
	
	class Loader : public xvk::Loader {
	public:
	
		// Required Instance Extensions
		std::vector<const char*> requiredInstanceExtensions {
			#ifdef _DEBUG
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			#endif
		};
		
		// Required Instance Layers
		std::vector<const char*> requiredInstanceLayers {
			#ifdef _DEBUG
			"VK_LAYER_LUNARG_standard_validation",
			#endif
		};
		
		void CheckExtensions(bool logging = false);
		void CheckLayers(bool logging = false);
		void CheckVulkanVersion();

	};
	
}
