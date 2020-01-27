#pragma once
#include "../common.h"

namespace v4d::graphics {
    using namespace glm;

    struct Vertex {
		alignas(16) vec3 pos;
		alignas(16) vec3 normal;
		alignas(16) vec3 color;
		
		static std::vector<VertexInputAttributeDescription> GetInputAttributes() {
			return {
				{0, offsetof(Vertex, pos), VK_FORMAT_R32G32B32A32_SFLOAT},
				{1, offsetof(Vertex, normal), VK_FORMAT_R32G32B32A32_SFLOAT},
				{2, offsetof(Vertex, color), VK_FORMAT_R32G32B32A32_SFLOAT},
			};
		}
    };
    
    struct PrimitiveGeometry {
        vec3 position {0};
        std::vector<Vertex> vertices {};
        std::vector<uint32_t> indices {};

        mat4 modelViewMatrix {1};
        StagedBuffer vertexBuffer {VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};
        StagedBuffer indexBuffer {VK_BUFFER_USAGE_INDEX_BUFFER_BIT};

        PrimitiveGeometry() {
            vertexBuffer.AddSrcDataPtr(&vertices);
            indexBuffer.AddSrcDataPtr(&indices);
        }

        void AllocateBuffers(Device* device, Queue transferQueue) {
            vertexBuffer.Allocate(device);
            indexBuffer.Allocate(device);

            auto cmdBuffer = device->BeginSingleTimeCommands(transferQueue);
                vertexBuffer.Update(device, cmdBuffer);
                indexBuffer.Update(device, cmdBuffer);
            device->EndSingleTimeCommands(transferQueue, cmdBuffer);
        }

        void FreeBuffers(Device* device) {
            vertexBuffer.Free(device);
            indexBuffer.Free(device);
        }

    };
}
