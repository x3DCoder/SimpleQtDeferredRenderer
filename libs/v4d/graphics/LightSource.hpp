#pragma once
#include "../common.h"
#include "Camera.hpp"

namespace v4d::graphics {
    using namespace glm;

    enum LightSourceType : int {
        POINT_LIGHT = 0,
        SPOT_LIGHT = 1,
    };

    struct LightSourcePushConstant {
        alignas(4)	LightSourceType type;
        alignas(16)	vec3 color;
        alignas(4)	float intensity;
        alignas(16)	vec3 viewPosition;
        alignas(4)	float innerAngle;
        alignas(16)	vec3 viewDirection;
        alignas(4)	float outerAngle;
        alignas(64) mat4 cameraViewToShadowMapMatrix {1};
    };

    struct LightSource { // used as push constant, so maximum size is 128 bytes
        LightSourceType type;
        dvec3 worldPosition;
        vec3 color;
        float intensity;
        vec3 worldDirection;
        float innerAngle;
        float outerAngle;

        LightSource(
            LightSourceType type, 
            dvec3 worldPosition, 
            vec3 color = vec3{1}, 
            float intensity = 1, 
            dvec3 worldDirection = dvec3{1},
            float innerAngle = 0, 
            float outerAngle = 0
        ) : 
            type(type),
            worldPosition(worldPosition), 
            color(color), 
            intensity(intensity), 
            worldDirection{worldDirection}, 
            innerAngle(innerAngle),
            outerAngle(max(innerAngle, outerAngle))
        {}

        mat4 MakeLightProjectionMatrix() {
            return mat4(Camera::MakeProjectionMatrix(outerAngle*2.0, 1.0, 0.5, 100.0));
        }

        mat4 MakeLightViewMatrix(const Camera& camera) {
            return mat4(lookAt(worldPosition, worldPosition + dvec3(normalize(worldDirection)), camera.viewUp));
        }

        LightSourcePushConstant MakePushConstantFromCamera(const Camera& camera) {
            return {
                type,
                color,
                intensity,
                camera.viewMatrix * vec4(worldPosition, 1),
                innerAngle,
                transpose(inverse(mat3(camera.viewMatrix))) * normalize(worldDirection),
                outerAngle,
                MakeLightProjectionMatrix() * MakeLightViewMatrix(camera) * glm::mat4(inverse(camera.viewMatrix))
            };
        }
    };

}
