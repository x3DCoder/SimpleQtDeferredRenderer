/*
 * Vulkan Shader file abstraction
 * Part of the Vulkan4D open-source game engine under the LGPL license - https://github.com/Vulkan4D
 * @author Olivier St-Laurent <olivier@xenon3d.com>
 * 
 * This class is an abstraction of individual shader files within a shader program
 */
#pragma once
#include "../../common.h"

namespace v4d::graphics::vulkan {

	struct ShaderInfo {
		std::string filepath;
		std::string entryPoint;
		VkSpecializationInfo* specializationInfo;
		
		ShaderInfo(const std::string& filepath, const std::string& entryPoint, VkSpecializationInfo* specializationInfo = nullptr);
		ShaderInfo(const std::string& filepath, VkSpecializationInfo* specializationInfo);
		ShaderInfo(const std::string& filepath);
		ShaderInfo(const char* filepath);
	};

	static std::unordered_map<std::string, VkShaderStageFlagBits> SHADER_TYPES {
		{"vert", VK_SHADER_STAGE_VERTEX_BIT},
		{"tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
		{"tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
		{"geom", VK_SHADER_STAGE_GEOMETRY_BIT},
		{"frag", VK_SHADER_STAGE_FRAGMENT_BIT},
		{"comp", VK_SHADER_STAGE_COMPUTE_BIT},
		{"mesh", VK_SHADER_STAGE_MESH_BIT_NV},
		{"task", VK_SHADER_STAGE_TASK_BIT_NV},
		{"rgen", VK_SHADER_STAGE_RAYGEN_BIT_NV},
		{"rint", VK_SHADER_STAGE_INTERSECTION_BIT_NV},
		{"rahit", VK_SHADER_STAGE_ANY_HIT_BIT_NV},
		{"rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV},
		{"rmiss", VK_SHADER_STAGE_MISS_BIT_NV},
		{"rcall", VK_SHADER_STAGE_CALLABLE_BIT_NV},
	};

	class Shader {
	private:

		std::string filepath; // path relative to executable
		std::string entryPoint; // usually "main"
		
		VkSpecializationInfo* specializationInfo;
		
		std::vector<char> bytecode; // contains the spv file contents
		
		VkShaderModule module = VK_NULL_HANDLE;
		
	public:
		std::string name; // the shader file name without directory nor extension
		std::string type; // string key in SHADER_TYPES
		
		VkPipelineShaderStageCreateInfo stageInfo;

		Shader(std::string filepath/*relative to executable*/, std::string entryPoint = "main", VkSpecializationInfo* specializationInfo = nullptr);
		
		VkShaderModule CreateShaderModule(Device* device, VkPipelineShaderStageCreateFlags flags = 0);
		void DestroyShaderModule(Device* device);

	};
}
