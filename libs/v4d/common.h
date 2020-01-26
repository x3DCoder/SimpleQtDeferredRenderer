#pragma once

// std
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <string>
#include <algorithm>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <map>
#include <thread>
#include <queue>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <stdio.h>
#include <regex>
#include <vector>

#ifdef _WIN32
	#include <windows.h>
	#include <io.h>
#else
    #include <dlfcn.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

// Logging
#define LOG(msg) std::cout << " " << msg << std::endl;
#define LOG_WARN(msg) std::cout << "WARNING: " << msg << std::endl;
#define LOG_ERROR(msg) std::cerr << "ERROR: " << msg << std::endl;

// v4d/graphics/vulkan
#include "graphics/vulkan/Loader.h"
#include "graphics/vulkan/PhysicalDevice.h"
#include "graphics/vulkan/Device.h"
#include "graphics/vulkan/Instance.h"
#include "graphics/vulkan/Image.h"
#include "graphics/vulkan/SwapChain.h"
#include "graphics/vulkan/Buffer.h"
#include "graphics/vulkan/DescriptorSet.h"
#include "graphics/vulkan/PipelineLayout.h"
#include "graphics/vulkan/Shader.h"
#include "graphics/vulkan/ShaderProgram.h"
#include "graphics/vulkan/RenderPass.h"
#include "graphics/vulkan/ShaderPipeline.h"
#include "graphics/vulkan/ComputeShaderPipeline.h"
#include "graphics/vulkan/RasterShaderPipeline.h"

// v4d/graphics
#include "graphics/Renderer.h"

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
