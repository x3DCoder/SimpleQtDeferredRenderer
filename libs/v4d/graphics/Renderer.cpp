#include "libs/v4d/common.h"

using namespace v4d::graphics;

#pragma region Configuration Methods

void Renderer::RequiredDeviceExtension(const char* ext) {
	requiredDeviceExtensions.push_back(std::move(ext));
}

void Renderer::OptionalDeviceExtension(const char* ext) {
	optionalDeviceExtensions.push_back(std::move(ext));
}

bool Renderer::IsDeviceExtensionEnabled(const char* ext) {
	return enabledDeviceExtensions.find(ext) != enabledDeviceExtensions.end();
}

#pragma endregion

#pragma region Virtual INIT Methods

void Renderer::CreateDevices() {
	// Select The Best Main PhysicalDevice using a score system
	renderingPhysicalDevice = SelectSuitablePhysicalDevice([this](int& score, PhysicalDevice* physicalDevice){
		// Build up a score here and the PhysicalDevice with the highest score will be selected.

		// Mandatory physicalDevice requirements for rendering graphics
		if (!physicalDevice->QueueFamiliesContainsFlags(VK_QUEUE_GRAPHICS_BIT, 1, surface))
			return;
		// User-defined required extensions
		for (auto& ext : requiredDeviceExtensions) if (!physicalDevice->SupportsExtension(ext))
			return;
		
		score = 1;
		
		// Each Optional extensions adds one point to the score
		for (auto& ext : optionalDeviceExtensions) if (physicalDevice->SupportsExtension(ext))
			++score;

		// User-defined score function
		ScorePhysicalDeviceSelection(score, physicalDevice);
	});

	LOG("Selected Rendering PhysicalDevice: " << renderingPhysicalDevice->GetDescription());

	// Prepare Device Features (remove unsupported features from list of features to enable)
	auto supportedDeviceFeatures = renderingPhysicalDevice->GetFeatures();
	const size_t deviceFeaturesArraySize = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
	VkBool32 supportedDeviceFeaturesData[deviceFeaturesArraySize];
	VkBool32 appDeviceFeaturesData[deviceFeaturesArraySize];
	memcpy(supportedDeviceFeaturesData, &supportedDeviceFeatures, sizeof(VkPhysicalDeviceFeatures));
	memcpy(appDeviceFeaturesData, &supportedDeviceFeatures, sizeof(VkPhysicalDeviceFeatures));
	for (size_t i = 0; i < deviceFeaturesArraySize; ++i) {
		if (appDeviceFeaturesData[i] && !supportedDeviceFeaturesData[i]) {
			appDeviceFeaturesData[i] = VK_FALSE;
		}
	}
	memcpy(&deviceFeatures, appDeviceFeaturesData, sizeof(VkPhysicalDeviceFeatures));
	
	// Prepare enabled extensions
	deviceExtensions.clear();
	for (auto& ext : requiredDeviceExtensions) {
		deviceExtensions.push_back(ext);
		enabledDeviceExtensions[ext] = true;
		LOG("Enabling Device Extension: " << ext)
	}
	for (auto& ext : optionalDeviceExtensions) {
		if (renderingPhysicalDevice->SupportsExtension(ext)) {
			deviceExtensions.push_back(ext);
			enabledDeviceExtensions[ext] = true;
			LOG("Enabling Device Extension: " << ext)
		} else {
			enabledDeviceExtensions[ext] = false;
		}
	}
	
	// Create Logical Device
	renderingDevice = new Device(
		renderingPhysicalDevice,
		deviceFeatures,
		deviceExtensions,
		vulkanLoader->requiredInstanceLayers,
		{// Queues
			{
				"graphics",
				VK_QUEUE_GRAPHICS_BIT,
				1, // Count
				{1.0f}, // Priorities (one per queue count)
				surface // Putting a surface here forces the need for a presentation feature on that specific queue family
			},
			{
				"transfer",
				VK_QUEUE_TRANSFER_BIT,
			},
		}
	);

	// Get Queues
	graphicsQueue = renderingDevice->GetQueue("graphics");
	transferQueue = renderingDevice->GetQueue("transfer");
	presentQueue = graphicsQueue;
	
	if (presentQueue.handle == nullptr) {
		throw std::runtime_error("Failed to get Presentation Queue for surface");
	}
}

void Renderer::DestroyDevices() {
	delete renderingDevice;
}

void Renderer::CreateSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	dynamicRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	graphicsFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Initialize in the signaled state so that we dont get stuck on the first frame

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (renderingDevice->CreateSemaphore(&semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphore for ImageAvailable");
		}
		if (renderingDevice->CreateSemaphore(&semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphore for RenderFinished");
		}
		if (renderingDevice->CreateSemaphore(&semaphoreInfo, nullptr, &dynamicRenderFinishedSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphore for RenderFinished");
		}
		if (renderingDevice->CreateFence(&fenceInfo, nullptr, &graphicsFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphicsFences");
		}
	}
}

void Renderer::DestroySyncObjects() {
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		renderingDevice->DestroySemaphore(imageAvailableSemaphores[i], nullptr);
		renderingDevice->DestroySemaphore(renderFinishedSemaphores[i], nullptr);
		renderingDevice->DestroySemaphore(dynamicRenderFinishedSemaphores[i], nullptr);
		renderingDevice->DestroyFence(graphicsFences[i], nullptr);
	}
}

void Renderer::CreateCommandPools() {
	renderingDevice->CreateCommandPool(graphicsQueue.familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &graphicsQueue.commandPool);
	renderingDevice->CreateCommandPool(transferQueue.familyIndex, 0, &transferQueue.commandPool);
}

void Renderer::DestroyCommandPools() {
	renderingDevice->DestroyCommandPool(graphicsQueue.commandPool);
	renderingDevice->DestroyCommandPool(transferQueue.commandPool);
}

void Renderer::CreateDescriptorSets() {
	for (auto* set : descriptorSets) {
		set->CreateDescriptorSetLayout(renderingDevice);
	}
	
	// Descriptor sets / pool
	std::map<VkDescriptorType, uint> descriptorTypes {};
	for (auto* set : descriptorSets) {
		for (auto&[binding, descriptor] : set->GetBindings()) {
			if (descriptorTypes.find(descriptor.descriptorType) == descriptorTypes.end()) {
				descriptorTypes[descriptor.descriptorType] = 1;
			} else {
				descriptorTypes[descriptor.descriptorType]++;
			}
		}
	}
	
	if (descriptorSets.size() > 0) {
		renderingDevice->CreateDescriptorPool(
			descriptorTypes,
			descriptorPool,
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		);
		
		// Allocate descriptor sets
		std::vector<VkDescriptorSetLayout> setLayouts {};
		vkDescriptorSets.resize(descriptorSets.size());
		setLayouts.reserve(descriptorSets.size());
		for (auto* set : descriptorSets) {
			setLayouts.push_back(set->GetDescriptorSetLayout());
		}
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = (uint)setLayouts.size();
		allocInfo.pSetLayouts = setLayouts.data();
		if (renderingDevice->AllocateDescriptorSets(&allocInfo, vkDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor sets");
		}
		for (int i = 0; i < descriptorSets.size(); ++i) {
			descriptorSets[i]->descriptorSet = vkDescriptorSets[i];
		}
	}
	
	UpdateDescriptorSets();
}

void Renderer::DestroyDescriptorSets() {
	// Descriptor Sets
	if (descriptorSets.size() > 0) {
		renderingDevice->FreeDescriptorSets(descriptorPool, (uint)vkDescriptorSets.size(), vkDescriptorSets.data());
		for (auto* set : descriptorSets) set->DestroyDescriptorSetLayout(renderingDevice);
		// Descriptor pools
		renderingDevice->DestroyDescriptorPool(descriptorPool, nullptr);
	}
}

void Renderer::UpdateDescriptorSets() {
	if (descriptorSets.size() > 0) {
		std::vector<VkWriteDescriptorSet> descriptorWrites {};
		for (auto* set : descriptorSets) {
			for (auto&[binding, descriptor] : set->GetBindings()) {
				if (descriptor.IsWriteDescriptorSetValid()) {
					descriptorWrites.push_back(descriptor.GetWriteDescriptorSet(set->descriptorSet));
				}
			}
		}
		renderingDevice->UpdateDescriptorSets((uint)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::UpdateDescriptorSets(std::vector<DescriptorSet*>&& setsToUpdate) {
	if (setsToUpdate.size() > 0) {
		std::vector<VkWriteDescriptorSet> descriptorWrites {};
		for (auto* set : setsToUpdate) {
			for (auto&[binding, descriptor] : set->GetBindings()) {
				if (descriptor.IsWriteDescriptorSetValid()) {
					descriptorWrites.push_back(descriptor.GetWriteDescriptorSet(set->descriptorSet));
				}
			}
		}
		renderingDevice->UpdateDescriptorSets((uint)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

bool Renderer::CreateSwapChain() {
	std::scoped_lock lock(renderingMutex, lowPriorityRenderingMutex);
	
	// Put old swapchain in a temporary pointer and delete it after creating new swapchain
	SwapChain* oldSwapChain = swapChain;

	// Create the new swapchain object
	swapChain = new SwapChain(
		renderingDevice,
		surface,
		{ // Preferred Extent (Screen Resolution)
			0, // width
			0 // height
		},
		preferredFormats,
		preferredPresentModes
	);
	// Assign queues
	swapChain->AssignQueues({presentQueue.familyIndex, graphicsQueue.familyIndex});
	
	if (swapChain->extent.width == 0 || swapChain->extent.height == 0) {
		return false;
	}

	// Create the swapchain handle and corresponding imageviews
	swapChain->Create(oldSwapChain);

	// Destroy the old swapchain
	if (oldSwapChain != nullptr) {
		oldSwapChain->Destroy();
		delete oldSwapChain;
	}
	
	return true;
}

void Renderer::DestroySwapChain() {
	swapChain->Destroy();
	delete swapChain;
	swapChain = nullptr;
}

void Renderer::CreateCommandBuffers() {
	std::scoped_lock lock(renderingMutex, lowPriorityRenderingMutex);
	
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					/*	VK_COMMAND_BUFFER_LEVEL_PRIMARY = Can be submitted to a queue for execution, but cannot be called from other command buffers
						VK_COMMAND_BUFFER_LEVEL_SECONDARY = Cannot be submitted directly, but can be called from primary command buffers
					*/
	
	{// Graphics Commands Recorder
		// Because one of the drawing commands involves binding the right VkFramebuffer, we'll actually have to record a command buffer for every image in the swap chain once again.
		// Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup
		graphicsCommandBuffers.resize(swapChain->imageViews.size());
		
		allocInfo.commandPool = graphicsQueue.commandPool;
		allocInfo.commandBufferCount = (uint) graphicsCommandBuffers.size();
		if (renderingDevice->AllocateCommandBuffers(&allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers");
		}

		// Starting command buffer recording...
		for (size_t i = 0; i < graphicsCommandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // We have used this flag because we may already be scheduling the drawing commands for the next frame while the last frame is not finished yet.
							/*	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = The command buffer will be rerecorded right after executing it once
								VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT = This is a secondary command buffer that will be entirely within a single render pass.
								VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT = The command buffer can be resubmited while it is also already pending execution
							*/
			beginInfo.pInheritanceInfo = nullptr; // only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers.
			// If a command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it.
			// It's not possible to append commands to a buffer at a later time.
			if (renderingDevice->BeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("Faild to begin recording command buffer");
			}
			
			// Record commands
			RecordGraphicsCommandBuffer(graphicsCommandBuffers[i], i);
			
			if (renderingDevice->EndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to record command buffer");
			}
		}
	}
	graphicsDynamicCommandBuffers.resize(graphicsCommandBuffers.size());
	if (renderingDevice->AllocateCommandBuffers(&allocInfo, graphicsDynamicCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}
	
}

void Renderer::DestroyCommandBuffers() {
	renderingDevice->FreeCommandBuffers(graphicsQueue.commandPool, static_cast<uint32_t>(graphicsCommandBuffers.size()), graphicsCommandBuffers.data());
	renderingDevice->FreeCommandBuffers(graphicsQueue.commandPool, static_cast<uint32_t>(graphicsDynamicCommandBuffers.size()), graphicsDynamicCommandBuffers.data());
}

#pragma endregion

#pragma region Helper methods

VkCommandBuffer Renderer::BeginSingleTimeCommands(Queue queue) {
	return renderingDevice->BeginSingleTimeCommands(queue);
}

void Renderer::EndSingleTimeCommands(Queue queue, VkCommandBuffer commandBuffer) {
	renderingDevice->EndSingleTimeCommands(queue, commandBuffer);
}

void Renderer::AllocateBufferStaged(Queue queue, Buffer& buffer) {
	auto cmdBuffer = BeginSingleTimeCommands(queue);
	Buffer stagingBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer.srcDataPointers = std::ref(buffer.srcDataPointers);
	stagingBuffer.Allocate(renderingDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
	buffer.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer.Allocate(renderingDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
	Buffer::Copy(renderingDevice, cmdBuffer, stagingBuffer.buffer, buffer.buffer, buffer.size);
	EndSingleTimeCommands(queue, cmdBuffer);
	stagingBuffer.Free(renderingDevice);
}
void Renderer::AllocateBuffersStaged(Queue queue, std::vector<Buffer>& buffers) {
	auto cmdBuffer = BeginSingleTimeCommands(queue);
	std::vector<Buffer> stagingBuffers {};
	stagingBuffers.reserve(buffers.size());
	for (auto& buffer : buffers) {
		auto& stagingBuffer = stagingBuffers.emplace_back(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer.srcDataPointers = std::ref(buffer.srcDataPointers);
		stagingBuffer.Allocate(renderingDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
		buffer.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer.Allocate(renderingDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
		Buffer::Copy(renderingDevice, cmdBuffer, stagingBuffer.buffer, buffer.buffer, buffer.size);
	}
	EndSingleTimeCommands(queue, cmdBuffer);
	for (auto& stagingBuffer : stagingBuffers) {
		stagingBuffer.Free(renderingDevice);
	}
}
void Renderer::AllocateBuffersStaged(Queue queue, std::vector<Buffer*>& buffers) {
	auto cmdBuffer = BeginSingleTimeCommands(queue);
	std::vector<Buffer> stagingBuffers {};
	stagingBuffers.reserve(buffers.size());
	for (auto& buffer : buffers) {
		auto& stagingBuffer = stagingBuffers.emplace_back(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer.srcDataPointers = std::ref(buffer->srcDataPointers);
		stagingBuffer.Allocate(renderingDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
		buffer->usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer->Allocate(renderingDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
		Buffer::Copy(renderingDevice, cmdBuffer, stagingBuffer.buffer, buffer->buffer, buffer->size);
	}
	EndSingleTimeCommands(queue, cmdBuffer);
	for (auto& stagingBuffer : stagingBuffers) {
		stagingBuffer.Free(renderingDevice);
	}
}

void Renderer::AllocateBufferStaged(Buffer& buffer) {
	AllocateBufferStaged(transferQueue, buffer);
}
void Renderer::AllocateBuffersStaged(std::vector<Buffer>& buffers) {
	AllocateBuffersStaged(transferQueue, buffers);
}
void Renderer::AllocateBuffersStaged(std::vector<Buffer*>& buffers) {
	AllocateBuffersStaged(transferQueue, buffers);
}

void Renderer::TransitionImageLayout(Image image, VkImageLayout oldLayout, VkImageLayout newLayout) {
	auto commandBuffer = BeginSingleTimeCommands(graphicsQueue);
	TransitionImageLayout(commandBuffer, image.image, oldLayout, newLayout, image.mipLevels, image.arrayLayers);
	EndSingleTimeCommands(graphicsQueue, commandBuffer);
}
void Renderer::TransitionImageLayout(VkCommandBuffer commandBuffer, Image image, VkImageLayout oldLayout, VkImageLayout newLayout) {
	TransitionImageLayout(commandBuffer, image.image, oldLayout, newLayout, image.mipLevels, image.arrayLayers);
}
void Renderer::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount) {
	auto commandBuffer = BeginSingleTimeCommands(graphicsQueue);
	TransitionImageLayout(commandBuffer, image, oldLayout, newLayout, mipLevels, layerCount);
	EndSingleTimeCommands(graphicsQueue, commandBuffer);
}
void Renderer::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount) {
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout; // VK_IMAGE_LAYOUT_UNDEFINED if we dont care about existing contents of the image
	barrier.newLayout = newLayout;
	// If we are using the barrier to transfer queue family ownership, these two fields should be the indices of the queue families. Otherwise VK_QUEUE_FAMILY_IGNORED
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	//
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	//
	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	
	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldLayout) {
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			barrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newLayout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (barrier.srcAccessMask == 0)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	renderingDevice->CmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	auto commandBuffer = BeginSingleTimeCommands(graphicsQueue);
	CopyBufferToImage(commandBuffer, buffer, image, width, height);
	EndSingleTimeCommands(graphicsQueue, commandBuffer);
}
void Renderer::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	renderingDevice->CmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void Renderer::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels) {
	VkFormatProperties formatProperties;
	renderingPhysicalDevice->GetPhysicalDeviceFormatProperties(imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("Texture image format does not support linear blitting");
	}

	auto commandBuffer = BeginSingleTimeCommands(graphicsQueue);

	VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		renderingDevice->CmdPipelineBarrier(
			commandBuffer, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth>1? mipWidth/2 : 1, mipHeight>1? mipHeight/2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

		renderingDevice->CmdBlitImage(
			commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR
		);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		renderingDevice->CmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	renderingDevice->CmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(graphicsQueue, commandBuffer);
}

#pragma endregion

#pragma region Init/Load/Reset Methods

void Renderer::RecreateSwapChains() {
	std::scoped_lock lock(renderingMutex, lowPriorityRenderingMutex);
	
	if (graphicsLoadedToDevice)
		UnloadGraphicsFromDevice();
	
	// Re-Create the SwapChain
	if (!CreateSwapChain()) {
		return;
	}
	
	LoadGraphicsToDevice();
}

void Renderer::InitRenderer() {
	Init();
	InitLayouts();
	ConfigureShaders();
}

void Renderer::LoadRenderer() {
	std::scoped_lock lock(renderingMutex, lowPriorityRenderingMutex);
	
	CreateDevices();
	Info();
	CreateSyncObjects();
	if (!CreateSwapChain()) {
		return;
	}
	
	LoadGraphicsToDevice();
}

void Renderer::UnloadRenderer() {
	std::scoped_lock lock(renderingMutex, lowPriorityRenderingMutex);
	
	if (graphicsLoadedToDevice)
		UnloadGraphicsFromDevice();
	
	DestroySwapChain();
	DestroySyncObjects();
	DestroyDevices();
}

void Renderer::ReloadRenderer() {
	if (renderThreadId != std::this_thread::get_id()) {
		mustReload = true;
		return;
	}
	
	std::scoped_lock lock(renderingMutex, lowPriorityRenderingMutex);
	
	mustReload = false;
	
	LOG("Reloading renderer...")
	
	if (graphicsLoadedToDevice)
		UnloadGraphicsFromDevice();
	
	DestroySwapChain();
	DestroySyncObjects();
	DestroyDevices();
	
	ReadShaders();
	
	CreateDevices();
	Info();
	CreateSyncObjects();
	
	if (!CreateSwapChain()) {
		return;
	}
	
	LoadGraphicsToDevice();
}

void Renderer::LoadGraphicsToDevice() {
	CreateCommandPools();
	CreateResources();
	AllocateBuffers();
	CreateDescriptorSets();
	CreatePipelines(); // shaders are assigned here
	CreateCommandBuffers(); // objects are rendered here
	
	graphicsLoadedToDevice = true;
}

void Renderer::UnloadGraphicsFromDevice() {
	// Wait for renderingDevice to be idle before destroying everything
	renderingDevice->DeviceWaitIdle(); // We can also wait for operations in a specific command queue to be finished with vkQueueWaitIdle. These functions can be used as a very rudimentary way to perform synchronization. 

	DestroyCommandBuffers();
	DestroyPipelines();
	DestroyDescriptorSets();
	FreeBuffers();
	DestroyResources();
	DestroyCommandPools();
	
	graphicsLoadedToDevice = false;
}

#pragma endregion

#pragma region Constructor & Destructor

Renderer::Renderer(Loader* loader, const char* applicationName, uint applicationVersion, Window* window)
: Instance(loader, applicationName, applicationVersion, true) {
	surface = window->CreateVulkanSurface(handle);
}

Renderer::~Renderer() {
	DestroySurfaceKHR(surface, nullptr);
}

#pragma endregion

#pragma region Public Methods

void Renderer::Render() {
	std::lock_guard lock(renderingMutex);
	renderThreadId = std::this_thread::get_id();
	
	if (!graphicsLoadedToDevice) {
		RecreateSwapChains();
		return;
	}
	
	if (mustReload) {
		ReloadRenderer();
		return;
	}
	
	uint64_t timeout = 1000UL * 1000 * 1000 * 30; // 30 seconds

	// Get an image from the swapchain
	uint imageIndex;
	VkResult result = renderingDevice->AcquireNextImageKHR(
		swapChain->GetHandle(), // swapChain
		timeout, // timeout in nanoseconds (using max disables the timeout)
		imageAvailableSemaphores[currentFrameInFlight], // semaphore
		VK_NULL_HANDLE, // fence
		&imageIndex // output the index of the swapchain image in there
	);

	// Check for errors
	if (result == VK_ERROR_OUT_OF_DATE_KHR || !graphicsLoadedToDevice) {
		// SwapChain is out of date, for instance if the window was resized, stop here and ReCreate the swapchain.
		RecreateSwapChains();
		return;
	} else if (result == VK_SUBOPTIMAL_KHR) {
		LOG("Swapchain is suboptimal...")
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to acquire swap chain images");
	}

	// Update data every frame
	FrameUpdate(imageIndex);
	
	std::array<VkSubmitInfo, 2> graphicsSubmitInfo {};
		graphicsSubmitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		graphicsSubmitInfo[1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	std::array<VkSemaphore, 1> graphicsWaitSemaphores {
		imageAvailableSemaphores[currentFrameInFlight],
	};
	VkPipelineStageFlags graphicsWaitStages[] {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	{// Configure Graphics
		renderingDevice->WaitForFences(1, &graphicsFences[currentFrameInFlight], VK_TRUE, timeout);
		renderingDevice->ResetFences(1, &graphicsFences[currentFrameInFlight]);
		renderingDevice->ResetCommandBuffer(graphicsDynamicCommandBuffers[imageIndex], 0);
		if (renderingDevice->BeginCommandBuffer(graphicsDynamicCommandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Faild to begin recording command buffer");
		}
		RunDynamicGraphics(graphicsDynamicCommandBuffers[imageIndex]);
		if (renderingDevice->EndCommandBuffer(graphicsDynamicCommandBuffers[imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
		// dynamic commands
		graphicsSubmitInfo[0].waitSemaphoreCount = 0;
		graphicsSubmitInfo[0].pWaitSemaphores = nullptr;
		graphicsSubmitInfo[0].pWaitDstStageMask = nullptr;
		graphicsSubmitInfo[0].commandBufferCount = 1;
		graphicsSubmitInfo[0].pCommandBuffers = &graphicsDynamicCommandBuffers[imageIndex];
		graphicsSubmitInfo[0].signalSemaphoreCount = 1;
		graphicsSubmitInfo[0].pSignalSemaphores = &dynamicRenderFinishedSemaphores[currentFrameInFlight];
		// static commands
		graphicsSubmitInfo[1].waitSemaphoreCount = graphicsWaitSemaphores.size();
		graphicsSubmitInfo[1].pWaitSemaphores = graphicsWaitSemaphores.data();
		graphicsSubmitInfo[1].pWaitDstStageMask = graphicsWaitStages;
		graphicsSubmitInfo[1].commandBufferCount = 1;
		graphicsSubmitInfo[1].pCommandBuffers = &graphicsCommandBuffers[imageIndex];
		graphicsSubmitInfo[1].signalSemaphoreCount = 1;
		graphicsSubmitInfo[1].pSignalSemaphores = &renderFinishedSemaphores[currentFrameInFlight];
	}
	
	// Submit Graphics
	result = renderingDevice->QueueSubmit(graphicsQueue.handle, graphicsSubmitInfo.size(), graphicsSubmitInfo.data(), graphicsFences[currentFrameInFlight]);
	if (result != VK_SUCCESS) {
		if (result == VK_ERROR_DEVICE_LOST) {
			LOG_WARN("Render() Failed to submit graphics command buffer : VK_ERROR_DEVICE_LOST. Reloading renderer...")
			ReloadRenderer();
			return;
		}
		LOG_ERROR((int)result)
		throw std::runtime_error("Render() Failed to submit graphics command buffer");
	}

	// Present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	// Specify which semaphore to wait on before presentation can happen
	presentInfo.waitSemaphoreCount = 2;
	VkSemaphore presentWaitSemaphores[] = {
		renderFinishedSemaphores[currentFrameInFlight],
		dynamicRenderFinishedSemaphores[currentFrameInFlight],
	};
	presentInfo.pWaitSemaphores = presentWaitSemaphores;
	// Specify the swap chains to present images to and the index for each swap chain. (almost always a single one)
	VkSwapchainKHR swapChains[] = {swapChain->GetHandle()};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	// The next param allows to specify an array of VkResult values to check for every individual swap chain if presentation was successful.
	// its not necessary if only using a single swap chain, because we can simply use the return value of the present function.
	presentInfo.pResults = nullptr;
	// Send the present info to the presentation queue !
	// This submits the request to present an image to the swap chain.
	result = renderingDevice->QueuePresentKHR(presentQueue.handle, &presentInfo);

	// Check for errors
	if (result == VK_ERROR_OUT_OF_DATE_KHR || !graphicsLoadedToDevice) {
		// SwapChain is out of date, for instance if the window was resized, stop here and ReCreate the swapchain.
		RecreateSwapChains();
		return;
	} else if (result == VK_SUBOPTIMAL_KHR) {
		LOG("Swapchain is suboptimal...")
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain images");
	}

	// Increment currentFrameInFlight
	currentFrameInFlight = (currentFrameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;
}

#pragma endregion
