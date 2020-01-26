#pragma once
#include "libs/v4d/common.h"

#include "libs/v4d/graphics/Camera.hpp"
#include "libs/v4d/graphics/LightSource.hpp"

using namespace v4d::graphics;

class DeferredRenderer : public Renderer {
    using Renderer::Renderer; // use parent constructor

public: // Scene
	Camera camera;
	std::vector<LightSource> lightSources {};

private: // Shaders

    PipelineLayout rasterizationLayout, lightingLayout;

    RasterShaderPipeline primitivesShader {rasterizationLayout, {
        "shaders/primitives.vert",
        "shaders/primitives.frag",
    }};

    RasterShaderPipeline lightingShader {lightingLayout, {
        "shaders/lighting.vert",
        "shaders/lighting.frag",
    }};

private: // Render passes
    RenderPass rasterizationPass, lightingPass;

private: // Images
	DepthStencilImage depthImage {};
	Image gBuffer_albedo { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ,1,1, { VK_FORMAT_R32G32B32A32_SFLOAT }};
	Image gBuffer_normal { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ,1,1, { VK_FORMAT_R16G16B16_SNORM, VK_FORMAT_R16G16B16A16_SNORM }};
	Image gBuffer_position { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ,1,1, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT }};

private: // Buffers
	StagedBuffer cameraUniformBuffer {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Camera)};

private: // Init
    void Init() override {
		cameraUniformBuffer.AddSrcDataPtr(&camera, sizeof(Camera));
	}
    void ScorePhysicalDeviceSelection(int&, PhysicalDevice*) override {}

	void InitLayouts() override {
		// Base descriptor set containing Camera and such
		auto* baseDescriptorSet_0 = descriptorSets.emplace_back(new DescriptorSet(0));
		baseDescriptorSet_0->AddBinding_uniformBuffer(0, &cameraUniformBuffer.deviceLocalBuffer, VK_SHADER_STAGE_ALL_GRAPHICS);

		// Rasterization
		rasterizationLayout.AddDescriptorSet(baseDescriptorSet_0);

		// Lighting
		auto* gBuffersDescriptorSet_1 = descriptorSets.emplace_back(new DescriptorSet(1));
		gBuffersDescriptorSet_1->AddBinding_inputAttachment(0, &gBuffer_albedo.view, VK_SHADER_STAGE_FRAGMENT_BIT);
		gBuffersDescriptorSet_1->AddBinding_inputAttachment(1, &gBuffer_normal.view, VK_SHADER_STAGE_FRAGMENT_BIT);
		gBuffersDescriptorSet_1->AddBinding_inputAttachment(2, &gBuffer_position.view, VK_SHADER_STAGE_FRAGMENT_BIT);
		lightingLayout.AddDescriptorSet(baseDescriptorSet_0);
		lightingLayout.AddDescriptorSet(gBuffersDescriptorSet_1);
		lightingLayout.AddPushConstant<LightSource>(VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	void ConfigureShaders() override {
		// Rasterization Pass
		primitivesShader.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lightingShader.depthStencilState.depthTestEnable = VK_TRUE;
		lightingShader.depthStencilState.depthWriteEnable = VK_TRUE;
        lightingShader.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		
		// Lighting Pass
		lightingShader.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		lightingShader.depthStencilState.depthTestEnable = VK_FALSE;
		lightingShader.depthStencilState.depthWriteEnable = VK_FALSE;
		lightingShader.rasterizer.cullMode = VK_CULL_MODE_NONE;
		lightingShader.SetData(3);
	}
	
private: // Resources

	void CreateResources() override {
		depthImage.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		gBuffer_albedo.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		gBuffer_normal.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		gBuffer_position.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
	}
	
	void DestroyResources() override {
		depthImage.Destroy(renderingDevice);
		gBuffer_albedo.Destroy(renderingDevice);
		gBuffer_normal.Destroy(renderingDevice);
		gBuffer_position.Destroy(renderingDevice);
	}
	
	void AllocateBuffers() override {
		cameraUniformBuffer.Allocate(renderingDevice);
	}
	
	void FreeBuffers() override {
		cameraUniformBuffer.Free(renderingDevice);
	}

private: // Pipelines

	void CreatePipelines() override {
		
	}
	
	void DestroyPipelines() override {
		
	}
	
private: // Commands

	void RecordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, int imageIndex) override {
		// Transition swapchain image to present layout
		TransitionImageLayout(commandBuffer, swapChain->images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}
	
    void RunDynamicGraphics(VkCommandBuffer) override {
		
	}
	
public: // Scene configuration
	
	void ReadShaders() override {
		primitivesShader.ReadShaders();
		lightingShader.ReadShaders();
	}
	
	void LoadScene() override {
		
	}
	
	void UnloadScene() override {
		
	}
	
public: // Update

    void FrameUpdate(uint) override {
		
	}
	
};
