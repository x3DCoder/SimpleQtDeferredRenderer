/*
 * Vulkan shader-related Pipeline abstraction
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 * 
 * This class handles the binding and draw calls related to a specific shader program
 */
#pragma once
#include "../../common.h"

namespace v4d::graphics::vulkan {

	class ShaderPipeline : public ShaderProgram {
	public:
		using ShaderProgram::ShaderProgram; // use parent constructor
		virtual ~ShaderPipeline();
		
		virtual void CreatePipeline(Device*) = 0;
		virtual void DestroyPipeline(Device*) = 0;
		
		// Execute() will call Bind() and Render() automatically
		virtual void Execute(Device* device, VkCommandBuffer cmdBuffer, uint32_t instanceCount, void* pushConstant, int pushConstantIndex = 0);
		virtual void Execute(Device* device, VkCommandBuffer cmdBuffer);
		
		// sends a push constant to the shader now
		void PushConstant(Device* device, VkCommandBuffer cmdBuffer, void* pushConstant, int pushConstantIndex = 0);
		
	protected:
		VkPipeline pipeline = VK_NULL_HANDLE;
		
		// binds the pipeline (to be implemented in child classes)
		virtual void Bind(Device*, VkCommandBuffer) = 0;
		
		// issues the draw calls (to be implemented in child classes)
		virtual void Render(Device*, VkCommandBuffer, uint32_t instanceCount) = 0;
		
	};
	
}
