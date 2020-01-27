#pragma once
#include "libs/v4d/common.h"

#include "libs/v4d/graphics/Camera.hpp"
#include "libs/v4d/graphics/LightSource.hpp"
#include "libs/v4d/graphics/PrimitiveGeometry.hpp"

using namespace v4d::graphics;

class DeferredRenderer : public Renderer {
    using Renderer::Renderer; // use parent constructor

public: // Scene
	Camera camera;
	std::vector<LightSource> lightSources {};
	std::vector<PrimitiveGeometry> sceneObjects {};

	StagedBuffer cameraUBO {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Camera::viewMatrix)};

private: // Shaders

    PipelineLayout rasterizationLayout, lightingLayout;

    RasterShaderPipeline primitivesShader {rasterizationLayout, {
        "shaders/primitives.vert",
        "shaders/primitives.frag",
    }};

    RasterShaderPipeline shadowMapShader {rasterizationLayout, {
        "shaders/primitives.shadow.vert",
        "shaders/primitives.shadow.frag",
    }};

    RasterShaderPipeline skyboxShader {rasterizationLayout, {
        "shaders/skybox.vert",
        "shaders/skybox.geom",
        "shaders/skybox.frag",
    }};

    RasterShaderPipeline lightingShader {lightingLayout, {
        "shaders/lighting.vert",
        "shaders/lighting.frag",
    }};

private: // Render passes
    RenderPass rasterizationPass, shadowPass, skyboxPass, lightingPass;

private: // Images
	Image gBuffer_albedo { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ,1,1, { VK_FORMAT_R32G32B32A32_SFLOAT }};
	Image gBuffer_normal { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ,1,1, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT }};
	Image gBuffer_position { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ,1,1, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT }};
	DepthStencilImage depthStencilImage {};
	
	DepthImage spotLightShadowMap { VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT };
	int shadowMapSize = 1024;

	CubeMapImage skybox {};
	int skyboxSize = 1024;

	std::vector<VkClearValue> clearValues{4};

private: // Init
    void Init() override {
		cameraUBO.AddSrcDataPtr(&camera.viewMatrix, sizeof(Camera::viewMatrix));
	}
    void ScorePhysicalDeviceSelection(int&, PhysicalDevice*) override {}

	void InitLayouts() override {
		// Base descriptor set containing Camera and such
		auto* baseDescriptorSet_0 = descriptorSets.emplace_back(new DescriptorSet(0));
        baseDescriptorSet_0->AddBinding_uniformBuffer(0, &cameraUBO.deviceLocalBuffer, VK_SHADER_STAGE_FRAGMENT_BIT);

		// Rasterization
		rasterizationLayout.AddDescriptorSet(baseDescriptorSet_0);
        rasterizationLayout.AddPushConstant<PrimitiveGeometry::MVP>(VK_SHADER_STAGE_VERTEX_BIT);

		// Lighting
		auto* gBuffersDescriptorSet_1 = descriptorSets.emplace_back(new DescriptorSet(1));
		gBuffersDescriptorSet_1->AddBinding_inputAttachment(0, &gBuffer_albedo.view, VK_SHADER_STAGE_FRAGMENT_BIT);
		gBuffersDescriptorSet_1->AddBinding_inputAttachment(1, &gBuffer_normal.view, VK_SHADER_STAGE_FRAGMENT_BIT);
		gBuffersDescriptorSet_1->AddBinding_inputAttachment(2, &gBuffer_position.view, VK_SHADER_STAGE_FRAGMENT_BIT);
		gBuffersDescriptorSet_1->AddBinding_combinedImageSampler(3, &spotLightShadowMap, VK_SHADER_STAGE_FRAGMENT_BIT);
		gBuffersDescriptorSet_1->AddBinding_combinedImageSampler(4, &skybox, VK_SHADER_STAGE_FRAGMENT_BIT);
		lightingLayout.AddDescriptorSet(baseDescriptorSet_0);
		lightingLayout.AddDescriptorSet(gBuffersDescriptorSet_1);
		lightingLayout.AddPushConstant<LightSourcePushConstant>(VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	void ConfigureShaders() override {
		// Rasterization Pass
		primitivesShader.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		primitivesShader.depthStencilState.depthTestEnable = VK_TRUE;
		primitivesShader.depthStencilState.depthWriteEnable = VK_TRUE;
        primitivesShader.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        primitivesShader.AddVertexInputBinding(sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, Vertex::GetInputAttributes());

		// Shadow map
		shadowMapShader.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		shadowMapShader.depthStencilState.depthTestEnable = VK_TRUE;
		shadowMapShader.depthStencilState.depthWriteEnable = VK_TRUE;
        shadowMapShader.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        shadowMapShader.AddVertexInputBinding(sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX, Vertex::GetInputAttributes());
		
		// Skybox
		skyboxShader.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		skyboxShader.depthStencilState.depthTestEnable = VK_FALSE;
		skyboxShader.depthStencilState.depthWriteEnable = VK_FALSE;
        skyboxShader.rasterizer.cullMode = VK_CULL_MODE_NONE;
		skyboxShader.SetData(6);
		
		// Lighting Pass
		lightingShader.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		lightingShader.depthStencilState.depthTestEnable = VK_FALSE;
		lightingShader.depthStencilState.depthWriteEnable = VK_FALSE;
		lightingShader.rasterizer.cullMode = VK_CULL_MODE_NONE;
		lightingShader.SetData(3);
	}
	
private: // Resources

	void CreateResources() override {
		depthStencilImage.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		gBuffer_albedo.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		gBuffer_normal.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		gBuffer_position.Create(renderingDevice, swapChain->extent.width, swapChain->extent.height);
		spotLightShadowMap.Create(renderingDevice, shadowMapSize, shadowMapSize);
		skybox.Create(renderingDevice, skyboxSize, skyboxSize);
	}
	
	void DestroyResources() override {
		depthStencilImage.Destroy(renderingDevice);
		gBuffer_albedo.Destroy(renderingDevice);
		gBuffer_normal.Destroy(renderingDevice);
		gBuffer_position.Destroy(renderingDevice);
		spotLightShadowMap.Destroy(renderingDevice);
		skybox.Destroy(renderingDevice);
	}
	
	void AllocateBuffers() override {
		cameraUBO.Allocate(renderingDevice);

		for (auto& obj : sceneObjects) {
			obj.AllocateBuffers(renderingDevice, transferQueue);
		}
	}
	
	void FreeBuffers() override {
        cameraUBO.Free(renderingDevice);

		for (auto& obj : sceneObjects) {
			obj.FreeBuffers(renderingDevice);
		}
	}

private: // Pipelines

	void CreatePipelines() override {
		lightingLayout.Create(renderingDevice);
		rasterizationLayout.Create(renderingDevice);

		const std::array<Image*, 3> gBuffers {
			&gBuffer_albedo,
			&gBuffer_normal,
			&gBuffer_position,
		};

		{// Rasterization pass

			std::array<Image*, 4> images {
				&gBuffer_albedo,
				&gBuffer_normal,
				&gBuffer_position,
				&depthStencilImage,
			};

			std::array<VkAttachmentDescription, images.size()> attachments {};
			std::array<VkAttachmentReference, gBuffers.size()> colorAttachmentRefs {};
			VkAttachmentReference depthStencilAttachment;

			int attachmentsIndex = 0;
			int colorAttachmentsIndex = 0;

			// Add gBuffers as color attachments and depth/stencil attachment
			for (auto* image : images) {
				if (image->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
					attachments[attachmentsIndex].format = image->format;
					attachments[attachmentsIndex].samples = VK_SAMPLE_COUNT_1_BIT;
					attachments[attachmentsIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					attachments[attachmentsIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					attachments[attachmentsIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachments[attachmentsIndex].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					colorAttachmentRefs[colorAttachmentsIndex++] = {
						rasterizationPass.AddAttachment(attachments[attachmentsIndex]),
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
					};
				} else if (image->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
					attachments[attachmentsIndex].format = image->format;
					attachments[attachmentsIndex].samples = VK_SAMPLE_COUNT_1_BIT;
					attachments[attachmentsIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					attachments[attachmentsIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					attachments[attachmentsIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					attachments[attachmentsIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					attachments[attachmentsIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachments[attachmentsIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthStencilAttachment = {
						rasterizationPass.AddAttachment(attachments[attachmentsIndex]),
						VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
					};
				}
				attachmentsIndex++;
			}
			
			// SubPass
			VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = colorAttachmentRefs.size();
				subpass.pColorAttachments = colorAttachmentRefs.data();
				subpass.pDepthStencilAttachment = &depthStencilAttachment;
			rasterizationPass.AddSubpass(subpass);
			
			// Create the render pass
			rasterizationPass.Create(renderingDevice);
			rasterizationPass.CreateFrameBuffers(renderingDevice, images.data(), images.size());
			
			// Shader
			primitivesShader.SetRenderPass(&gBuffer_albedo, rasterizationPass.handle, 0);
			for (size_t i = 0; i < gBuffers.size(); ++i)
				primitivesShader.AddColorBlendAttachmentState(VK_FALSE);
			primitivesShader.CreatePipeline(renderingDevice);
		}
		
		{// Shadow pass
			VkAttachmentDescription depthAttachment {};
			depthAttachment.format = spotLightShadowMap.format;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

			VkAttachmentReference depthAttachmentRef {
				shadowPass.AddAttachment(depthAttachment),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};
			
			// SubPass
			VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.pDepthStencilAttachment = &depthAttachmentRef;
			shadowPass.AddSubpass(subpass);
			
			// Create the render pass
			shadowPass.Create(renderingDevice);
			shadowPass.CreateFrameBuffers(renderingDevice, spotLightShadowMap);
			
			// Shader
			shadowMapShader.SetRenderPass(&spotLightShadowMap, shadowPass.handle, 0);
			shadowMapShader.CreatePipeline(renderingDevice);
		}
		
		{// Skybox pass
			VkAttachmentDescription colorAttachment {};
			colorAttachment.format = skybox.format;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

			VkAttachmentReference colorAttachmentRef {
                skyboxPass.AddAttachment(colorAttachment),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
			
			// SubPass
			VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;
			skyboxPass.AddSubpass(subpass);
			
			// Create the render pass
			skyboxPass.Create(renderingDevice);
			skyboxPass.CreateFrameBuffers(renderingDevice, skybox);
			
			// Shader
			skyboxShader.SetRenderPass(&skybox, skyboxPass.handle, 0);
			skyboxShader.AddColorBlendAttachmentState(VK_FALSE);
			skyboxShader.CreatePipeline(renderingDevice);
		}
		
		{// Lighting pass

			std::array<VkImageView, gBuffers.size() + 1> imageViews {
				gBuffer_albedo.view,
				gBuffer_normal.view,
				gBuffer_position.view,
				VK_NULL_HANDLE, // VK_NULL_HANDLE = the swapchain
			};

			std::array<VkAttachmentReference, 1> colorAttachmentRefs {};
			std::array<VkAttachmentReference, gBuffers.size()> inputAttachmentRefs {};
			std::array<VkAttachmentDescription, colorAttachmentRefs.size() + inputAttachmentRefs.size()> attachments {};

			int attachmentsIndex = 0;
			int colorAttachmentsIndex = 0;
			int inputAttachmentsIndex = 0;

			// Add gBuffers as input attachments
			for (auto* image : gBuffers) {
				attachments[attachmentsIndex].format = image->format;
				attachments[attachmentsIndex].samples = VK_SAMPLE_COUNT_1_BIT;
				attachments[attachmentsIndex].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				attachments[attachmentsIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachments[attachmentsIndex].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				attachments[attachmentsIndex].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				inputAttachmentRefs[inputAttachmentsIndex++] = {
					lightingPass.AddAttachment(attachments[attachmentsIndex]),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};
				attachmentsIndex++;
			}

			// Add SwapChain image as color attachment
			attachments[attachmentsIndex].format = swapChain->format.format;
			attachments[attachmentsIndex].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[attachmentsIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[attachmentsIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[attachmentsIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[attachmentsIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			colorAttachmentRefs[colorAttachmentsIndex++] = {
				lightingPass.AddAttachment(attachments[attachmentsIndex]),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
			attachmentsIndex++;

			// SubPass
			VkSubpassDescription subpass {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = colorAttachmentRefs.size();
				subpass.pColorAttachments = colorAttachmentRefs.data();
				subpass.inputAttachmentCount = inputAttachmentRefs.size();
				subpass.pInputAttachments = inputAttachmentRefs.data();
			lightingPass.AddSubpass(subpass);
			
			// Create the render pass
			lightingPass.Create(renderingDevice);
			lightingPass.CreateFrameBuffers(renderingDevice, swapChain, imageViews.data(), imageViews.size());
			
			// Shader
			lightingShader.SetRenderPass(swapChain, lightingPass.handle, 0);
			lightingShader.AddColorBlendAttachmentState(
				VK_TRUE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_OP_MAX
			);
			lightingShader.CreatePipeline(renderingDevice);
		}
		
	}
	
	void DestroyPipelines() override {
		// shader pipelines
		primitivesShader.DestroyPipeline(renderingDevice);
		shadowMapShader.DestroyPipeline(renderingDevice);
		skyboxShader.DestroyPipeline(renderingDevice);
		lightingShader.DestroyPipeline(renderingDevice);

		// frame buffers
		rasterizationPass.DestroyFrameBuffers(renderingDevice);
		shadowPass.DestroyFrameBuffers(renderingDevice);
		skyboxPass.DestroyFrameBuffers(renderingDevice);
		lightingPass.DestroyFrameBuffers(renderingDevice);

		// render passes
		rasterizationPass.Destroy(renderingDevice);
		shadowPass.Destroy(renderingDevice);
		skyboxPass.Destroy(renderingDevice);
		lightingPass.Destroy(renderingDevice);

		// layouts
		lightingLayout.Destroy(renderingDevice);
		rasterizationLayout.Destroy(renderingDevice);
	}
	
private: // Commands

	void RecordGraphicsCommandBuffer(VkCommandBuffer, int) override {}
	
    void RunDynamicGraphics(VkCommandBuffer commandBuffer, int imageIndex) override {
		cameraUBO.Update(renderingDevice, commandBuffer);

		// Render primitives
		rasterizationPass.Begin(renderingDevice, commandBuffer, gBuffer_albedo, clearValues);
		for (auto& obj : sceneObjects) {
			primitivesShader.SetData(&obj.vertexBuffer.deviceLocalBuffer, &obj.indexBuffer.deviceLocalBuffer, obj.indices.size());
			primitivesShader.Execute(renderingDevice, commandBuffer, 1, &obj.mvp);
		}
		rasterizationPass.End(renderingDevice, commandBuffer);

		// Shadow map
		for (auto& lightSource : lightSources) {
			if (lightSource.type == SPOT_LIGHT) {
				shadowPass.Begin(renderingDevice, commandBuffer, spotLightShadowMap, clearValues);
				for (auto& obj : sceneObjects) {
					PrimitiveGeometry::MVP mvp {
						// Make a new projection matrix and modelView matrix based on that spot light
						lightSource.MakeLightProjectionMatrix(),
						lightSource.MakeLightViewMatrix(camera) * glm::translate(glm::mat4(1), obj.position)
					};
					shadowMapShader.SetData(&obj.vertexBuffer.deviceLocalBuffer, &obj.indexBuffer.deviceLocalBuffer, obj.indices.size());
					shadowMapShader.Execute(renderingDevice, commandBuffer, 1, &mvp);
				}
				shadowPass.End(renderingDevice, commandBuffer);
				break; // We only support one shadow map for now, for one spot light
			}
		}

		// Generate Skybox
		skyboxPass.Begin(renderingDevice, commandBuffer, skybox, clearValues);
		skyboxShader.Execute(renderingDevice, commandBuffer);
		skyboxPass.End(renderingDevice, commandBuffer);

		// Lighting
		lightingPass.Begin(renderingDevice, commandBuffer, swapChain, clearValues, imageIndex);
		for (auto& lightSource : lightSources) {
			auto pushConstant = lightSource.MakePushConstantFromCamera(camera);
			lightingShader.Execute(renderingDevice, commandBuffer, 1, &pushConstant);
		}
		lightingPass.End(renderingDevice, commandBuffer);
	}
	
public: // Scene configuration
	
	void ReadShaders() override {
		primitivesShader.ReadShaders();
		shadowMapShader.ReadShaders();
		skyboxShader.ReadShaders();
		lightingShader.ReadShaders();
	}
	
	void LoadScene() override {
		// Light Sources
		lightSources.push_back({AMBIENT_SKYBOX, {0,0,0}, /*color*/{1,1,1}, /*intensity*/0.02});
		lightSources.push_back({POINT_LIGHT, /*position*/{ -8,-4, 10}, /*color*/{1,0,0}, /*intensity*/0.5});
		lightSources.push_back({POINT_LIGHT, /*position*/{ 18, 4,  2}, /*color*/{0,1,0}, /*intensity*/0.5});
		lightSources.push_back({POINT_LIGHT, /*position*/{-12, 0,  5}, /*color*/{0,0,1}, /*intensity*/0.5});
		lightSources.push_back({SPOT_LIGHT,  /*position*/{  0, 0, 20}, /*color*/{1,1,1}, /*intensity*/1.0, /*direction*/{-0.1,0.1,-1}, /*inner angle*/20, /*outer angle*/30});

		// Multicolor Triangle
		sceneObjects.push_back({
			// position
			{0, 5, 0},
			{ // Vertices
				{/*pos*/{-1, 0,-1}, /*normal*/{0,-1,0}, /*color*/{1,0,0}},
				{/*pos*/{ 1, 0,-1}, /*normal*/{0,-1,0}, /*color*/{0,1,0}},
				{/*pos*/{ 0, 1, 1}, /*normal*/{0,-1,0}, /*color*/{0,0,1}},
			},
			{ // Indices
				0,1,2,
			}
		});

		// Gray Cube
		sceneObjects.push_back({
			// position
			{3, 5, -2},

			{ // Vertices

				// front
				{/*pos*/{-1,-1,-1}, /*normal*/{0,-1,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1,-1,-1}, /*normal*/{0,-1,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1,-1, 1}, /*normal*/{0,-1,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1,-1, 1}, /*normal*/{0,-1,0}, /*color*/{0.5,0.5,0.5}},
				
				// back
				{/*pos*/{-1, 1,-1}, /*normal*/{0,1,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1, 1,-1}, /*normal*/{0,1,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1, 1, 1}, /*normal*/{0,1,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1, 1, 1}, /*normal*/{0,1,0}, /*color*/{0.5,0.5,0.5}},

				// right
				{/*pos*/{ 1,-1,-1}, /*normal*/{1,0,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1, 1,-1}, /*normal*/{1,0,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1, 1, 1}, /*normal*/{1,0,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1,-1, 1}, /*normal*/{1,0,0}, /*color*/{0.5,0.5,0.5}},

				// left
				{/*pos*/{-1,-1,-1}, /*normal*/{-1,0,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1, 1,-1}, /*normal*/{-1,0,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1, 1, 1}, /*normal*/{-1,0,0}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1,-1, 1}, /*normal*/{-1,0,0}, /*color*/{0.5,0.5,0.5}},
				
				// top
				{/*pos*/{-1,-1, 1}, /*normal*/{0,0,1}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1,-1, 1}, /*normal*/{0,0,1}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1, 1, 1}, /*normal*/{0,0,1}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1, 1, 1}, /*normal*/{0,0,1}, /*color*/{0.5,0.5,0.5}},

				// bottom
				{/*pos*/{-1,-1,-1}, /*normal*/{0,0,-1}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1,-1,-1}, /*normal*/{0,0,-1}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{ 1, 1,-1}, /*normal*/{0,0,-1}, /*color*/{0.5,0.5,0.5}},
				{/*pos*/{-1, 1,-1}, /*normal*/{0,0,-1}, /*color*/{0.5,0.5,0.5}},

			},
			{ // Indices
				0,1,2,  2,3,0, // front
				4,6,5,  4,7,6, // back
				8,9,10, 10,11,8, // right
				12,14,13, 12,15,14, // left
				16,17,18, 18,19,16, // top
				20,22,21, 20,23,22, // bottom
			}
		});

		// Greenish Plane
		sceneObjects.push_back({
			// position
			{0, 0, -6},
			{ // Vertices
				{/*pos*/{-1000,-1000,0}, /*normal*/{0,0,1}, /*color*/{0.4,0.5,0.1}},
				{/*pos*/{ 1000,-1000,0}, /*normal*/{0,0,1}, /*color*/{0.4,0.5,0.1}},
				{/*pos*/{ 1000, 1000,0}, /*normal*/{0,0,1}, /*color*/{0.4,0.5,0.1}},
				{/*pos*/{-1000, 1000,0}, /*normal*/{0,0,1}, /*color*/{0.4,0.5,0.1}},
			},
			{ // Indices
				0,1,2, 2,3,0
			}
		});

	}
	
	void UnloadScene() override {
		lightSources.clear();
		sceneObjects.clear();
	}
	
public: // Update

    void FrameUpdate(uint) override {
		// Update camera
		camera.RefreshProjectionMatrix((double) swapChain->extent.width / swapChain->extent.height);
		camera.RefreshViewMatrix();

		// Update objects matrices
		for (auto& obj : sceneObjects) {
			obj.SetMvpForCamera(camera);
		}
	}
	
};
