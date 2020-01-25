/*
 * Vulkan Compute pipeline abstraction
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 * 
 * This class extends from ShaderPipeline with specific functionality for compute shaders
 */
#pragma once
#include "../../common.h"

namespace v4d::graphics::vulkan {
	
	class ComputeShaderPipeline : public ShaderPipeline {
		uint32_t groupCountX = 0, groupCountY = 0, groupCountZ = 0;
		
	public:
		ComputeShaderPipeline(PipelineLayout& pipelineLayout, ShaderInfo shaderInfo);
		virtual ~ComputeShaderPipeline();
		
		virtual void CreatePipeline(Device* device) override;
		virtual void DestroyPipeline(Device* device) override;
		
		void SetGroupCounts(uint32_t x, uint32_t y, uint32_t z);
		
	protected:
		// these two methods are called automatically by Execute() from the parent class
		virtual void Bind(Device* device, VkCommandBuffer cmdBuffer) override;
		virtual void Render(Device* device, VkCommandBuffer cmdBuffer, uint32_t _unused_arg_ = 0) override;
	};
	
}
