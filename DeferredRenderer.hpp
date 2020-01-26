#pragma once
#include "libs/v4d/common.h"

using namespace v4d::graphics;

class DeferredRenderer : public Renderer {
private: 
	using Renderer::Renderer;

private: // Init
	void Init() override {
		
	}

	void ScorePhysicalDeviceSelection(int& score, PhysicalDevice* physicalDevice) override {
		
	}

	void InitLayouts() override {
		
	}
	
	void ConfigureShaders() override {
		
	}
	
private: // Resources

	void CreateResources() override {
		
	}
	
	void DestroyResources() override {
		
	}
	
	void AllocateBuffers() override {
		
	}
	
	void FreeBuffers() override {
		
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
	
	void RunDynamicGraphics(VkCommandBuffer commandBuffer) override {
		
	}
	
public: // Scene configuration
	
	void ReadShaders() override {
		
	}
	
	void LoadScene() override {
		
	}
	
	void UnloadScene() override {
		
	}
	
public: // Update

	void FrameUpdate(uint imageIndex) override {
		
	}
	
};
