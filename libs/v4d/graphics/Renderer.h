#pragma once
#include "libs/v4d/common.h"

#ifdef XVK_USE_QT_VULKAN_LOADER
    #include <QWindow>
#endif

using namespace v4d::graphics::vulkan;

namespace v4d::graphics {
    class Renderer : public Instance {
    protected: // class members

        // Main Render Surface
        VkSurfaceKHR surface;

        // Main Graphics Card
        PhysicalDevice* renderingPhysicalDevice = nullptr;
        Device* renderingDevice = nullptr;

        // Queues
        Queue graphicsQueue, presentQueue, transferQueue;

        // Command buffers
        std::vector<VkCommandBuffer> graphicsCommandBuffers, graphicsDynamicCommandBuffers;

        // Swap Chains
        SwapChain* swapChain = nullptr;

        // Synchronizations
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkSemaphore> dynamicRenderFinishedSemaphores;
        std::vector<VkFence> graphicsFences;
        size_t currentFrameInFlight = 0;
        const int MAX_FRAMES_IN_FLIGHT = 2;

        // States
        std::recursive_mutex renderingMutex, lowPriorityRenderingMutex;
        bool mustReload = false;
        bool graphicsLoadedToDevice = false;
        std::thread::id renderThreadId = std::this_thread::get_id();

        // Descriptor sets
        VkDescriptorPool descriptorPool;
        std::vector<DescriptorSet*> descriptorSets {};
        std::vector<VkDescriptorSet> vkDescriptorSets {};

    public: // Preferences
        std::vector<VkPresentModeKHR> preferredPresentModes {
            // VK_PRESENT_MODE_MAILBOX_KHR,	// TripleBuffering (No Tearing, low latency)
            // VK_PRESENT_MODE_FIFO_KHR,	// VSync ON (No Tearing, more latency)
            VK_PRESENT_MODE_IMMEDIATE_KHR,	// VSync OFF (With Tearing, no latency)
        };
        std::vector<VkSurfaceFormatKHR> preferredFormats {
            {VK_FORMAT_R32G32B32A32_SFLOAT, VK_COLOR_SPACE_HDR10_HLG_EXT},
            {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
        };

    private: // Device Extensions and features
        std::vector<const char*> requiredDeviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        std::vector<const char*> optionalDeviceExtensions {};
        std::vector<const char*> deviceExtensions {};
        std::unordered_map<std::string, bool> enabledDeviceExtensions {};

    public:
        VkPhysicalDeviceFeatures deviceFeatures {}; // This object will be modified to keep only the enabled values.

        void RequiredDeviceExtension(const char* ext);
        void OptionalDeviceExtension(const char* ext);
        bool IsDeviceExtensionEnabled(const char* ext);

    protected: // Pure-virtual methods

        // Init
        virtual void ScorePhysicalDeviceSelection(int& score, PhysicalDevice* physicalDevice) = 0;
        virtual void Init() = 0;
        virtual void InitLayouts() = 0;
        virtual void ConfigureShaders() = 0;

        // Resources
        virtual void CreateResources() = 0;
        virtual void DestroyResources() = 0;
        virtual void AllocateBuffers() = 0;
        virtual void FreeBuffers() = 0;

        // Scene
        virtual void LoadScene() = 0;
        virtual void UnloadScene() = 0;
        virtual void ReadShaders() = 0;

        // Pipelines
        virtual void CreatePipelines() = 0;
        virtual void DestroyPipelines() = 0;

        // Update
        virtual void FrameUpdate(uint imageIndex) = 0;

        // Commands
        virtual void RecordGraphicsCommandBuffer(VkCommandBuffer, int imageIndex) = 0;
        virtual void RunDynamicGraphics(VkCommandBuffer) = 0;

    protected: // Create/Destroy

        void CreateDevices();
        void DestroyDevices();

        void CreateSyncObjects();
        void DestroySyncObjects();

        void CreateCommandPools();
        void DestroyCommandPools();

        void CreateDescriptorSets();
        void DestroyDescriptorSets();

        bool CreateSwapChain();
        void DestroySwapChain();

        void CreateCommandBuffers();
        void DestroyCommandBuffers();

    public: // Descriptor sets
        void UpdateDescriptorSets();
        void UpdateDescriptorSets(std::vector<DescriptorSet*>&&);

    public: // Helpers

        VkCommandBuffer BeginSingleTimeCommands(Queue queue);
        void EndSingleTimeCommands(Queue queue, VkCommandBuffer commandBuffer);

        void AllocateBufferStaged(Queue queue, Buffer& buffer);
        void AllocateBuffersStaged(Queue queue, std::vector<Buffer>& buffers);
        void AllocateBuffersStaged(Queue queue, std::vector<Buffer*>& buffers);

        void AllocateBufferStaged(Buffer& buffer);
        void AllocateBuffersStaged(std::vector<Buffer>& buffers);
        void AllocateBuffersStaged(std::vector<Buffer*>& buffers);

        void TransitionImageLayout(Image image, VkImageLayout oldLayout, VkImageLayout newLayout);
        void TransitionImageLayout(VkCommandBuffer commandBuffer, Image image, VkImageLayout oldLayout, VkImageLayout newLayout);
        void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layerCount = 1);
        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layerCount = 1);

        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);

    protected: // Init/Reset Methods
        void RecreateSwapChains();

    public: // Init/Load Methods
        void InitRenderer();
        void LoadRenderer();
        void UnloadRenderer();
        void ReloadRenderer();

    protected:
        void LoadGraphicsToDevice();
        void UnloadGraphicsFromDevice();

    public:

        // Constructor & Destructor
        #ifdef XVK_USE_QT_VULKAN_LOADER
            Renderer(Loader* loader, QWindow* window);
        #else
            Renderer(Loader* loader, const char* applicationName, uint applicationVersion, QWindow* window);
        #endif
        ~Renderer() override;

        void Render();

    };
}
