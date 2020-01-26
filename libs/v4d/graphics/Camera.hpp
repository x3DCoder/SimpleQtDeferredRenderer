#pragma once
#include "../common.h"

namespace v4d::graphics {
    using namespace glm;
    
    struct Camera {

        alignas(4) int width = 0;
        alignas(4) int height = 0;
        alignas(32) dvec3 worldPosition {0};
        alignas(8) double fov = 70;
        alignas(32) dvec3 lookDirection {0,1,0};
        alignas(8) double znear = 0.001; // 1 mm
        alignas(32) dvec3 viewUp = {0,0,1};
        alignas(8) double zfar = 1.e16; // 1e16 = 1 light-year
        alignas(128) dmat4 viewMatrix {1};
        alignas(128) dmat4 projectionMatrix {1};

        void RefreshViewMatrix() {
            viewMatrix = lookAt(worldPosition, worldPosition + lookDirection, viewUp);
        }
        
        void RefreshProjectionMatrix() {
            // zfar and znear are swapped on purpose. 
            // this technique while also reversing the normal depth test operation will make the depth buffer linear again, giving it a better depth precision on the entire range. 
            projectionMatrix = perspective(radians(fov), (double) width / height, zfar, znear);
            projectionMatrix[1][1] *= -1;
        }
        
    };
}
