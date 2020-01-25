#include "../../common.h"

using namespace v4d::graphics::vulkan;

RasterShaderPipeline::RasterShaderPipeline(PipelineLayout& pipelineLayout, const std::vector<ShaderInfo>& shaderInfo)
 : ShaderPipeline(pipelineLayout, shaderInfo) {
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	colorBlending.logicOpEnable = VK_FALSE; // If enabled, will effectively replace and disable blendingAttachmentState.blendEnable
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // optional, if enabled above
	colorBlending.blendConstants[0] = 0; // optional
	colorBlending.blendConstants[1] = 0; // optional
	colorBlending.blendConstants[2] = 0; // optional
	colorBlending.blendConstants[3] = 0; // optional
	
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	
	// Optional
	// Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. 
	// The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality in common with an existing pipeline and switching between pipelines from the same parent can also be done quicker.
	// You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex. 
	// These are only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo.
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;
}

RasterShaderPipeline::~RasterShaderPipeline() {
	
}

void RasterShaderPipeline::SetData(Buffer* vertexBuffer, Buffer* indexBuffer, uint32_t indexCount) {
	this->vertexBuffer = vertexBuffer;
	this->indexBuffer = indexBuffer;
	this->vertexCount = 0;
	this->vertexOffset = 0;
	this->indexCount = indexCount > 0 ? indexCount : static_cast<uint32_t>(indexBuffer->size / sizeof(uint32_t));
	this->indexOffset = 0;
}

void RasterShaderPipeline::SetData(Buffer* vertexBuffer, VkDeviceSize vertexOffset, Buffer* indexBuffer, VkDeviceSize indexOffset, uint32_t indexCount) {
	this->vertexBuffer = vertexBuffer;
	this->indexBuffer = indexBuffer;
	this->vertexCount = 0;
	this->vertexOffset = vertexOffset;
	this->indexCount = indexCount;
	this->indexOffset = indexOffset;
}

void RasterShaderPipeline::SetData(Buffer* vertexBuffer, uint32_t vertexCount) {
	this->vertexBuffer = vertexBuffer;
	this->indexBuffer = nullptr;
	this->vertexCount = vertexCount;
	this->vertexOffset = 0;
	this->indexCount = 0;
	this->indexOffset = 0;
}

void RasterShaderPipeline::SetData(Buffer* vertexBuffer, VkDeviceSize vertexOffset, uint32_t vertexCount) {
	this->vertexBuffer = vertexBuffer;
	this->indexBuffer = nullptr;
	this->vertexCount = vertexCount;
	this->vertexOffset = vertexOffset;
	this->indexCount = 0;
	this->indexOffset = 0;
}

void RasterShaderPipeline::SetData(uint32_t vertexCount) {
	this->vertexBuffer = nullptr;
	this->indexBuffer = nullptr;
	this->vertexCount = vertexCount;
	this->vertexOffset = 0;
	this->indexCount = 0;
	this->indexOffset = 0;
}

void RasterShaderPipeline::CreatePipeline(Device* device) {
	CreateShaderStages(device);
	
	VkPipelineViewportStateCreateInfo viewportState {};
	VkViewport viewport {};
	VkRect2D scissor {};
	if (pipelineCreateInfo.pViewportState == nullptr) {
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float) renderTarget->width;
		viewport.height = (float) renderTarget->height;
		viewport.minDepth = 0;
		viewport.maxDepth = renderTarget->imageInfo.extent.depth;
		scissor.offset = {0, 0};
		scissor.extent = {renderTarget->width, renderTarget->height};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.pScissors = &scissor;
		pipelineCreateInfo.pViewportState = &viewportState;
	}
	
	// Bindings and Attributes
	vertexInputInfo.vertexBindingDescriptionCount = GetBindings()->size();
	vertexInputInfo.pVertexBindingDescriptions = GetBindings()->data();
	vertexInputInfo.vertexAttributeDescriptionCount = GetAttributes()->size();
	vertexInputInfo.pVertexAttributeDescriptions = GetAttributes()->data();

	// Dynamic states
	if (dynamicStates.size() > 0) {
		dynamicStateCreateInfo.dynamicStateCount = (uint)dynamicStates.size();
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	} else {
		pipelineCreateInfo.pDynamicState = nullptr;
	}
	
	pipelineCreateInfo.layout = GetPipelineLayout()->handle;

	// Fixed functions
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pRasterizationState = &rasterizer;
	pipelineCreateInfo.pMultisampleState = &multisampling;
	colorBlending.attachmentCount = colorBlendAttachments.size();
	colorBlending.pAttachments = colorBlendAttachments.data();
	pipelineCreateInfo.pColorBlendState = &colorBlending;

	// Shaders
	pipelineCreateInfo.stageCount = GetStages()->size();
	pipelineCreateInfo.pStages = GetStages()->data();
	
	// Create the actual pipeline
	if (device->CreateGraphicsPipelines(VK_NULL_HANDLE/*pipelineCache*/, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Graphics Pipeline");
	}
}

void RasterShaderPipeline::DestroyPipeline(Device* device) {
	device->DestroyPipeline(pipeline, nullptr);
	DestroyShaderStages(device);
	colorBlendAttachments.clear();
}

void RasterShaderPipeline::SetRenderPass(VkPipelineViewportStateCreateInfo* viewportState, VkRenderPass renderPass, uint32_t subpass) {
	pipelineCreateInfo.pViewportState = viewportState;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = subpass;
}

void RasterShaderPipeline::SetRenderPass(SwapChain* swapChain, VkRenderPass renderPass, uint32_t subpass) {
	pipelineCreateInfo.pViewportState = &swapChain->viewportState;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = subpass;
}

void RasterShaderPipeline::SetRenderPass(Image* renderTarget, VkRenderPass renderPass, uint32_t subpass) {
	this->renderTarget = renderTarget;
	pipelineCreateInfo.pViewportState = nullptr;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = subpass;
}

void RasterShaderPipeline::AddColorBlendAttachmentState(
	VkBool32 blendEnable,
	VkBlendFactor srcColorBlendFactor,
	VkBlendFactor dstColorBlendFactor,
	VkBlendOp colorBlendOp,
	VkBlendFactor srcAlphaBlendFactor,
	VkBlendFactor dstAlphaBlendFactor,
	VkBlendOp alphaBlendOp,
	VkColorComponentFlags colorWriteMask
) {
	colorBlendAttachments.push_back({
		blendEnable, // VkBool32
		srcColorBlendFactor, // VkBlendFactor
		dstColorBlendFactor, // VkBlendFactor
		colorBlendOp, // VkBlendOp
		srcAlphaBlendFactor, // VkBlendFactor
		dstAlphaBlendFactor, // VkBlendFactor
		alphaBlendOp, // VkBlendOp
		colorWriteMask // VkColorComponentFlags
	});
}

void RasterShaderPipeline::Bind(Device* device, VkCommandBuffer cmdBuffer) {
	device->CmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	GetPipelineLayout()->Bind(device, cmdBuffer);
}

void RasterShaderPipeline::Render(Device* device, VkCommandBuffer cmdBuffer, uint32_t instanceCount) {
	if (vertexBuffer == nullptr) {
		device->CmdDraw(cmdBuffer,
			vertexCount, // vertexCount
			instanceCount, // instanceCount
			0, // firstVertex (defines the lowest value of gl_VertexIndex)
			0  // firstInstance (defines the lowest value of gl_InstanceIndex)
		);
	} else {
		device->CmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer->buffer, &vertexOffset);
		if (indexBuffer == nullptr) {
			// Draw vertices
			device->CmdDraw(cmdBuffer,
				vertexCount, // vertexCount
				instanceCount, // instanceCount
				0, // firstVertex (defines the lowest value of gl_VertexIndex)
				0  // firstInstance (defines the lowest value of gl_InstanceIndex)
			);
		} else {
			// Draw indices
			device->CmdBindIndexBuffer(cmdBuffer, indexBuffer->buffer, indexOffset, VK_INDEX_TYPE_UINT32);
			device->CmdDrawIndexed(cmdBuffer,
				indexCount, // indexCount
				instanceCount, // instanceCount
				0, // firstIndex
				0, // vertexOffset (0 because we are already taking an offseted vertex buffer)
				0  // firstInstance (defines the lowest value of gl_InstanceIndex)
			);
		}
	}
}
