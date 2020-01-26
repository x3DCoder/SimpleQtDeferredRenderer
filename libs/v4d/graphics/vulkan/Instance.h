/*
 * Vulkan Instance abstraction
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 * 
 * This class extends from xvk's Vulkan Instance (xvk is another open-source library related to Vulkan4D)
 */
#pragma once
#include "../../common.h"

namespace v4d::graphics::vulkan {

	class Instance : public xvk::Interface::InstanceInterface {
	protected:
		vulkan::Loader* vulkanLoader;

	private:
		std::vector<PhysicalDevice*> availablePhysicalDevices;

		void LoadAvailablePhysicalDevices();

	public:
        #ifdef XVK_USE_QT_VULKAN_LOADER
            Instance(vulkan::Loader* loader);
        #else
            Instance(vulkan::Loader* loader, const char* applicationName, uint applicationVersion, bool logging = false);
        #endif

        virtual ~Instance();

		VkInstance GetHandle() const;

		PhysicalDevice* SelectSuitablePhysicalDevice(const std::function<void(int&, PhysicalDevice*)>& suitabilityFunc);

	};
}
